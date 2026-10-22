if exists("g:imp_loaded")
  finish
endif
let g:imp_loaded = v:true

command! ImpEnable call imp#enable()
command! ImpDisable call imp#disable()
command! ImpStatus call imp#status()
command! ImpToggle call imp#toggle()

augroup imp
  autocmd!

  if exists("##ModeChanged")
    autocmd ModeChanged * call imp#sync()
  else
    autocmd InsertEnter * call imp#sync()
    autocmd InsertLeave * call imp#sync()
  endif

  autocmd BufEnter * call imp#resync()
  autocmd FocusGained * call imp#resync()
augroup END
