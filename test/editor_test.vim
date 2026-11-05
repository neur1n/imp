set nomore
if empty($IMP_TEST_BACKEND) || !executable($IMP_TEST_BACKEND)
  set verbose=1
  echomsg 'IMP_TEST_BACKEND must point to the test backend executable'
  cquit
endif
let s:root = fnamemodify(resolve(expand('<sfile>:p')), ':h:h')
execute 'set runtimepath^=' . fnameescape(s:root)
let g:imp_backend = $IMP_TEST_BACKEND
runtime autoload/imp.vim

" Control only the target and backend timing; exercise the real queue and callbacks.
let s:prefix = matchstr(execute('function /target_mode'), '<SNR>\d\+_')
let g:ImpTestStart = funcref(s:prefix . 'start')
let g:ImpTestSyncDone = funcref(s:prefix . 'sync_done')
let g:ImpTestStatusDone = funcref(s:prefix . 'status_done')
execute 'function! ' . s:prefix . "target_mode() abort\n"
      \ . "return g:imp_test_target\nendfunction"
execute 'function! ' . s:prefix . "start(item) abort\n"
      \ . "call add(g:imp_test_started, a:item)\n"
      \ . "call call(g:ImpTestStart, [a:item])\nendfunction"
execute 'function! ' . s:prefix . "sync_done(item, ok, output, exit_code) abort\n"
      \ . "call assert_true(a:ok, join(a:output, ' | '))\n"
      \ . "call ImpTestCheckPair(a:item, a:output)\n"
      \ . "call add(g:imp_test_completed, a:item.args)\n"
      \ . "call call(g:ImpTestSyncDone, [a:item, a:ok, a:output, a:exit_code])\nendfunction"
execute 'function! ' . s:prefix . "status_done(item, ok, output, exit_code) abort\n"
      \ . "call assert_true(a:ok, join(a:output, ' | '))\n"
      \ . "call ImpTestCheckPair(a:item, a:output)\n"
      \ . "call add(g:imp_test_completed, a:item.args)\n"
      \ . "call call(g:ImpTestStatusDone, [a:item, a:ok, a:output, a:exit_code])\nendfunction"

function! ImpTestCheckPair(item, output) abort
  let l:index = index(a:item.args, '--pair')
  let l:pair = l:index >= 0 ? a:item.args[l:index + 1] : 'wetype'
  call assert_true(index(a:output, 'pair=' . l:pair) >= 0, 'backend received the captured pair')
  let l:index = index(a:item.args, '--config')
  let l:config = l:index >= 0 ? a:item.args[l:index + 1] : ''
  call assert_true(index(a:output, 'config=' . l:config) >= 0, 'backend received the captured config path')
endfunction

function! s:wait_for(expression) abort
  let l:start = reltime()
  while !eval(a:expression)
    if reltimefloat(reltime(l:start)) > 5
      throw 'Timed out: ' . a:expression
    endif
    sleep 5m
  endwhile
endfunction

function! s:reset(...) abort
  call imp#disable()
  let g:imp_pair = get(a:000, 0, '')
  let g:imp_config = get(a:000, 1, '')
  let g:imp_test_started = []
  let g:imp_test_completed = []
  let g:imp_test_target = 'alphanumeric'
  call imp#enable()
endfunction

function! s:request(mode) abort
  let g:imp_test_target = a:mode
  call imp#sync()
endfunction

function! s:complete(index) abort
  call s:wait_for('len(g:imp_test_started) > ' . a:index)
  let l:job = g:imp_test_started[a:index].job
  if has('nvim')
    call chansend(l:job, "continue\n")
  else
    call ch_sendraw(job_getchannel(l:job), "continue\n")
  endif
  call s:wait_for('len(g:imp_test_completed) > ' . a:index)
endfunction

function! s:expect(modes, message) abort
  let l:expected = []
  for l:mode in a:modes
    call add(l:expected, l:mode ==# 'status' ? ['status'] : ['set', l:mode])
  endfor
  call s:expect_requests(l:expected, a:message)
endfunction

function! s:expect_requests(requests, message) abort
  call assert_equal(a:requests, g:imp_test_completed, a:message)
  call assert_equal(len(a:requests), len(g:imp_test_started), a:message . ': extra job')
endfunction

try
  " Returning to the last applied mode must not skip the compensating request.
  call s:reset()
  call s:complete(0)
  call s:request('native')
  call s:request('alphanumeric')
  call s:complete(1)
  call s:complete(2)
  call s:expect(['alphanumeric', 'native', 'alphanumeric'], 'settled A -> N -> A')

  " The same transition must also work before the initial request completes.
  call s:reset()
  call s:request('native')
  call s:request('alphanumeric')
  call s:complete(0)
  call s:complete(1)
  call s:complete(2)
  call s:expect(['alphanumeric', 'native', 'alphanumeric'], 'inflight A -> N -> A')

  " A completion must not hide the opposite request that is now running.
  call s:reset()
  call s:complete(0)
  call s:request('native')
  call s:request('alphanumeric')
  call s:complete(1)
  call s:request('alphanumeric')
  call s:request('native')
  call s:complete(2)
  call s:complete(3)
  call s:expect(['alphanumeric', 'native', 'alphanumeric', 'native'], 'completion with pending set')

  " Deduplicate against a queued set even when status is the final queue item.
  call s:reset()
  call s:complete(0)
  call s:request('native')
  call s:request('alphanumeric')
  call imp#status()
  call s:request('alphanumeric')
  call s:complete(1)
  call s:request('alphanumeric')
  call s:complete(2)
  call s:request('alphanumeric')
  call s:complete(3)
  call s:expect(['alphanumeric', 'native', 'alphanumeric', 'status'], 'status preserves set order')

  " Deduplicate against the running set, or the applied mode during status.
  call s:reset()
  call imp#sync()
  call imp#status()
  call imp#sync()
  call s:complete(0)
  call imp#sync()
  call s:complete(1)
  call imp#sync()
  call s:expect(['alphanumeric', 'status'], 'repeated target')

  " Focus/buffer resync and repeated enable must bypass pending-set deduplication.
  call s:reset()
  call imp#resync()
  call imp#resync()
  call s:complete(0)
  call imp#enable()
  call s:complete(1)
  call s:complete(2)
  call s:complete(3)
  call imp#resync()
  call s:complete(4)
  call s:expect(repeat(['alphanumeric'], 5), 'forced resync and enable')

  call imp#disable()
  call imp#resync()
  call imp#sync()
  call s:expect(repeat(['alphanumeric'], 5), 'disabled sync')
  call imp#status()
  call s:complete(5)
  call s:expect(repeat(['alphanumeric'], 5) + ['status'], 'status while disabled')

  " Each queued set/status keeps its selected pair even if the global changes.
  call s:reset('pinyin')
  call imp#status()
  call s:request('native')
  let g:imp_pair = 'wubi'
  call imp#status()
  call s:request('native')
  call imp#sync()
  let g:imp_pair = 'pinyin'
  call imp#sync()
  let g:imp_pair = 'wetype'
  for s:index in range(6)
    call s:complete(s:index)
  endfor
  let s:requests = [
        \ ['--pair', 'pinyin', 'set', 'alphanumeric'],
        \ ['--pair', 'pinyin', 'status'],
        \ ['--pair', 'pinyin', 'set', 'native'],
        \ ['--pair', 'wubi', 'status'],
        \ ['--pair', 'wubi', 'set', 'native'],
        \ ['--pair', 'pinyin', 'set', 'native'],
        \ ]
  call s:expect_requests(s:requests, 'queued pair snapshots and pair-aware deduplication')

  " A settled same-mode request must still run if its pair changes.
  let g:imp_pair = 'pinyin'
  call imp#sync()
  call s:expect_requests(s:requests, 'same pair and mode are deduplicated')
  let g:imp_pair = 'wubi'
  call imp#sync()
  call s:complete(6)
  call add(s:requests, ['--pair', 'wubi', 'set', 'native'])
  call s:expect_requests(s:requests, 'changing the applied pair resends the same mode')

  let g:imp_pair = []
  try
    call imp#status()
    call assert_report('non-string pair must be rejected before starting a job')
  catch /^\[imp\] g:imp_pair must be a string$/
  endtry
  call s:expect_requests(s:requests, 'invalid pair type')

  " Config paths participate in deduplication and are captured before queueing.
  let s:config_a = fnamemodify('config a.conf', ':p')
  let s:config_b = fnamemodify('config b.conf', ':p')
  call s:reset('pinyin', 'config a.conf')
  call imp#status()
  let g:imp_config = 'config b.conf'
  call imp#sync()
  call imp#sync()
  call imp#status()
  let g:imp_config = 'config a.conf'
  call imp#sync()
  for s:index in range(5)
    call s:complete(s:index)
  endfor
  let s:requests = [
        \ ['--config', s:config_a, '--pair', 'pinyin', 'set', 'alphanumeric'],
        \ ['--config', s:config_a, '--pair', 'pinyin', 'status'],
        \ ['--config', s:config_b, '--pair', 'pinyin', 'set', 'alphanumeric'],
        \ ['--config', s:config_b, '--pair', 'pinyin', 'status'],
        \ ['--config', s:config_a, '--pair', 'pinyin', 'set', 'alphanumeric'],
        \ ]
  call s:expect_requests(s:requests, 'queued config snapshots')
  let g:imp_config = 'config b.conf'
  call imp#sync()
  call s:complete(5)
  call add(s:requests, ['--config', s:config_b, '--pair', 'pinyin', 'set', 'alphanumeric'])
  call s:expect_requests(s:requests, 'changing an applied config resends the same mode')
  call imp#resync()
  call s:complete(6)
  call add(s:requests, ['--config', s:config_b, '--pair', 'pinyin', 'set', 'alphanumeric'])
  call s:expect_requests(s:requests, 'resync rereads an unchanged config path')

  let g:imp_config = []
  try
    call imp#status()
    call assert_report('non-string config must be rejected before starting a job')
  catch /^\[imp\] g:imp_config must be a string$/
  endtry
  call s:expect_requests(s:requests, 'invalid config type')
catch
  call add(v:errors, v:exception . ' at ' . v:throwpoint)
finally
  call imp#disable()
endtry

if !empty(v:errors)
  set verbose=1
  for s:error in v:errors
    echomsg s:error
  endfor
  cquit
endif
qa!
