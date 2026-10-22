if exists("g:imp_autoload_loaded")
  finish
endif
let g:imp_autoload_loaded = v:true

let s:enabled = v:false
let s:queue = []
let s:current = {}
let s:last_applied_mode = ""
let s:last_requested_mode = ""

function! s:append_output(value) abort
  if empty(s:current)
    return
  endif

  if type(a:value) == v:t_list
    for l:line in a:value
      if !empty(l:line)
        call add(s:current.output, l:line)
      endif
    endfor
  elseif !empty(a:value)
    call add(s:current.output, a:value)
  endif
endfunction

function! s:backend_path() abort
  if exists("g:imp_backend") && !empty(g:imp_backend)
    return g:imp_backend
  endif

  let l:root = fnamemodify(resolve(expand("<script>:p")), ":h:h")
  let l:name = (has("win32") || has("win64")) ? "imp.exe" : "imp"
  return l:root . "/bin/" . l:name
endfunction

function! s:drain() abort
  if !empty(s:current) || empty(s:queue)
    return
  endif
  let l:item = remove(s:queue, 0)
  call s:start(l:item)
endfunction

function! s:enqueue(args, callback, label) abort
  call add(s:queue, {
        \ "args": a:args,
        \ "callback": a:callback,
        \ "label": a:label,
        \ "output": [],
        \ })
  call s:drain()
endfunction

function! s:finish(job, exit_code) abort
  if !s:is_current_job(a:job)
    return
  endif

  let l:item = s:current
  let l:output = copy(s:current.output)
  let s:current = {}
  let l:ok = a:exit_code == 0

  if !l:ok && empty(l:output)
    let l:output = ["backend exited with status " . a:exit_code]
  endif

  call call(l:item.callback, [l:item, l:ok, l:output, a:exit_code])
  call s:drain()
endfunction

function! s:is_current_channel(channel) abort
  if empty(s:current)
    return 0
  endif
  return get(s:current, "channel", v:null) ==# a:channel
endfunction

function! s:is_current_job(job) abort
  if empty(s:current)
    return 0
  endif
  return get(s:current, "job", v:null) ==# a:job
endfunction

function! s:nvim_exit(job, exit_code, event) abort
  call s:finish(a:job, a:exit_code)
endfunction

function! s:nvim_stderr(job, data, event) abort
  if s:is_current_job(a:job)
    call s:append_output(a:data)
  endif
endfunction

function! s:nvim_stdout(job, data, event) abort
  if s:is_current_job(a:job)
    call s:append_output(a:data)
  endif
endfunction

function! s:start(item) abort
  let l:command = [s:backend_path()] + a:item.args

  if has("nvim")
    let l:job = jobstart(l:command, {
          \ "on_stdout": function("s:nvim_stdout"),
          \ "on_stderr": function("s:nvim_stderr"),
          \ "on_exit": function("s:nvim_exit"),
          \ "stdout_buffered": v:true,
          \ "stderr_buffered": v:true,
          \ })
  elseif exists("*job_start")
    let l:job = job_start(l:command, {
          \ "out_cb": function("s:vim_stdout"),
          \ "err_cb": function("s:vim_stderr"),
          \ "exit_cb": function("s:vim_exit"),
          \ "close_cb": function("s:vim_close"),
          \ })
  else
    let l:job = 0
  endif

  " Vim returns a Job object; Neovim returns a numeric job id.
  let l:failed = type(l:job) == v:t_number && l:job <= 0
  if l:failed
    let s:current = {}
    call call(a:item.callback, [a:item, 0,
          \ ["could not start backend: " . s:backend_path()], 1])
    call s:drain()
    return
  endif

  let a:item.job = l:job
  if !has("nvim")
    let a:item.channel = job_getchannel(l:job)
  endif
  let s:current = a:item
endfunction

function! s:status_done(item, ok, output, exit_code) abort
  if a:ok
    echo "[imp] " . join(a:output, " | ")
  else
    echoerr "[imp] " . join(a:output, " | ")
  endif
endfunction

function! s:sync_done(item, ok, output, exit_code) abort
  if a:ok
    let s:last_applied_mode = a:item.mode
    let s:last_requested_mode = ""
    return
  endif

  let s:last_requested_mode = ""
  echoerr "[imp] " . join(a:output, " | ")
endfunction

function! s:target_mode() abort
  " Only Insert and Replace modes request native input.
  return strpart(mode(1), 0, 1) =~# "[iR]" ? "native" : "alphanumeric"
endfunction

function! s:vim_close(channel) abort
  if !s:is_current_channel(a:channel)
    return
  endif

  let l:job = s:current.job
  let l:exit_code = get(s:current, "exit_code", -1)
  if l:exit_code < 0 && exists("*job_info")
    let l:exit_code = get(job_info(l:job), "exitval", 1)
  endif
  call s:finish(l:job, l:exit_code)
endfunction

function! s:vim_exit(job, exit_code) abort
  if s:is_current_job(a:job)
    let s:current.exit_code = a:exit_code
  endif
endfunction

function! s:vim_stderr(channel, line) abort
  if s:is_current_channel(a:channel)
    call s:append_output(a:line)
  endif
endfunction

function! s:vim_stdout(channel, line) abort
  if s:is_current_channel(a:channel)
    call s:append_output(a:line)
  endif
endfunction

function! imp#disable() abort
  let s:enabled = v:false
  let s:queue = []
  let s:last_applied_mode = ""
  let s:last_requested_mode = ""

  if !empty(s:current)
    if has("nvim")
      call jobstop(s:current.job)
    elseif exists("*job_stop")
      call job_stop(s:current.job)
    endif

    let s:current = {}
  endif
endfunction

function! imp#enable() abort
  let s:enabled = v:true
  let s:last_applied_mode = ""
  let s:last_requested_mode = ""

  call imp#sync()
endfunction

function! imp#resync() abort
  let s:last_applied_mode = ""
  let s:last_requested_mode = ""

  call imp#sync()
endfunction

function! imp#status() abort
  call s:enqueue(["status"], function("s:status_done"), "status")
endfunction

function! imp#sync() abort
  if !s:enabled
    return
  endif

  let l:mode = s:target_mode()
  if l:mode ==# s:last_applied_mode || l:mode ==# s:last_requested_mode
    return
  endif

  let s:last_requested_mode = l:mode
  let l:item = {
        \ "mode": l:mode,
        \ "args": ["set", l:mode],
        \ "callback": function("s:sync_done"),
        \ "label": "sync",
        \ "output": [],
        \ }

  call add(s:queue, l:item)
  call s:drain()
endfunction

function! imp#toggle() abort
  if s:enabled
    call imp#disable()
    echo "[imp] Disabled"
  else
    call imp#enable()
    echo "[imp] Enabled"
  endif
endfunction
