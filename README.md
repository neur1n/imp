<pre align="center">
█▀▀▀▀▀█▀▀▀▀▀▀▀█▀▀▀▀▀▀▀█
█▄▄ ▄▄█  ▄ ▄  █   ▄▄  █
███ ███  █ █  █       █
█▀▀ ▀▀█  █ █  █   █████
█▄▄▄▄▄█▄▄█▄█▄▄█▄▄▄█████
</pre>

<p align="center">
  Input Method Pilot — an input-state manager.
</p>

<details open=true>
  <summary>Table of Contents</summary>
  <ul>
    <li><a href="#features">Features</a></li>
    <li><a href="#requirements">Requirements</a></li>
    <li><a href="#installation">Installation</a></li>
    <li><a href="#usage">Usage</a></li>
    <li><a href="#configuration">Configuration</a></li>
    <li><a href="#cli">CLI</a></li>
    <li><a href="#platform-support">Platform Support</a></li>
    <li><a href="#development">Development</a></li>
    <li><a href="#license">License</a></li>
  </ul>
</details>

## Features
- Switches input states to match Vim and Neovim editing modes.
- Works independently of Vim and Neovim as a command-line tool, including in
  your own scripts.

Supported platforms and input sources:
- Linux: WIP.
- macOS: WeType (微信输入法) and ABC.
- Windows: WIP.

See [Platform Support](#platform-support) for details.

## Requirements
### Build Tools
- CMake 3.20 or newer and a C++17-capable compiler.

### Vim / Neovim Integration
- Vim with job and channel support, or Neovim with `jobstart()` support.
- Neovim 0.12 or newer only if using the `vim.pack` installation method.
- No editor is required for standalone CLI use.

### Operating system
- Linux: WIP
- macOS: Xcode Command Line Tools or an equivalent Objective-C++ toolchain and
  macOS SDK; WeType and ABC installed and enabled.
- Windows: WIP.

## Installation
### Build and Install the CLI
Clone [neur1n/imp](https://github.com/neur1n/imp), then run these commands from
the repository root:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build
cmake --install build --prefix "$PWD"
```

This installs `bin/imp`.

### Manual Editor Integration
Add the repository to your `runtimepath`:

```vim
set runtimepath+=/absolute/path/to/imp
```

### Neovim: vim.pack
Add to your `init.lua` (Neovim 0.12+):

```lua
vim.pack.add({"https://github.com/neur1n/imp"})
```

Run the [build commands](#build-and-install-the-cli) in the installed plugin
directory after installation and updates. Find that directory with `:lua
print(vim.pack.get({"imp"})[1].path)`.

### Neovim: lazy.nvim
Add this plugin specification to your lazy.nvim configuration. The build
callback compiles and installs `bin/imp` on installation and updates:

```lua
{
  "neur1n/imp",
  cmd = {"ImpEnable", "ImpDisable", "ImpStatus", "ImpToggle"},
  build = function(plugin)
    local build = plugin.dir .. "/build"
    for _, command in ipairs({
      {"cmake", "-S", plugin.dir, "-B", build, "-DCMAKE_BUILD_TYPE=Release", "-DBUILD_TESTING=OFF"},
      {"cmake", "--build", build, "--config", "Release"},
      {"cmake", "--install", build, "--config", "Release", "--prefix", plugin.dir},
    }) do
      local output = vim.fn.system(command)
      if vim.v.shell_error ~= 0 then
        error(output)
      end
    end
  end,
}
```

## Usage
Automatic switching is off by default. Use these commands in Vim or Neovim:

```vim
" Switch now and follow editor modes
:ImpEnable
" Disable automatic switching
:ImpDisable
" Toggle automatic switching
:ImpToggle
" Query input state, even while disabled
:ImpStatus
```

| Editor mode | Requested input state |
| --- | --- |
| Insert / Replace | `native` |
| All other modes | `alphanumeric` |

While enabled, imp also reapplies the requested input state after focus and
buffer changes. Disabling it does not restore the previous input source.

## Configuration
The frontend defaults to `bin/imp` inside the plugin directory. To use another
executable, set its absolute path:

```vim
let g:imp_backend = '/absolute/path/to/imp/build/imp'
```

Or in Lua:

```lua
vim.g.imp_backend = "/absolute/path/to/imp/build/imp"
```

## CLI
Use the executable directly, or add its directory to `PATH`:

```sh
imp status
imp set native
imp set alphanumeric
imp --help
imp --version
```

`status` prints `key=value` lines: `backend`, `mode`, and, when available,
`source_id` and `source_name`. `set` prints the requested mode, such as
`mode=native`, on success. `mode=unknown` means the current state could not be
mapped; it is not itself an error.

Success exits with `0`; failures exit with `EXIT_FAILURE` and print `error:
[domain:code] message` to `stderr`, without a success result on `stdout`.

## Platform Support
### macOS
The `macos-tis` backend switches between registered input-source pairs using
TIS. The built-in pair is:

| Input state | Input source | Source ID |
| --- | --- | --- |
| `native` | WeType (微信输入法) | `com.tencent.inputmethod.wetype.pinyin` |
| `alphanumeric` | ABC | `com.apple.keylayout.ABC` |

This changes the selected source, not the input method's internal conversion
mode; selecting WeType does not guarantee Chinese mode. No keyboard events are
simulated.

The backend uses the pair containing the current source, falling back to the
first pair if none matches. An unavailable target source causes an error. Add
pairs in [`src/platform/macos_backend.mm`](src/platform/macos_backend.mm) and
rebuild; runtime configuration is not supported.

### Linux / Windows
WIP: Linux IBus/Fcitx5 and Windows target-thread TSF queries and mode control
are not implemented. Operations return POSIX `ENOSYS`.

### Editor integration
The editor-mode mapping is fixed. Automatic switching uses `ModeChanged` when
available, otherwise `InsertEnter` / `InsertLeave`; the fallback does not track
every mode transition.

## Development
Build with tests enabled and run them:

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The backend interface is documented in
[`inc/imp/backend.h`](inc/imp/backend.h); platform implementations live in
`src/platform/`.

## License
Distributed under the [MulanPSL-2.0](http://license.coscl.org.cn/MulanPSL2)
license. See [LICENSE](LICENSE) for details.

The bundled [`thirdparty/x.h`](thirdparty/x.h) carries its own copyright and
MulanPSL-2.0 license notice.

<details>
  <summary>Why MulanPSL-2.0?</summary>

The Mulan Permissive Software License v2 (MulanPSL-2.0) may be less familiar than more widely used licenses. To provide clarity and context, the following table (cited from [*Choose a License*](https://choosealicense.com/appendix/)) compares key aspects of MulanPSL v2 with popular licenses including *Apache-2.0*, *BSD-3-Clause*, and *MIT*.

| License          | Commercial Use | Distribution | Modification | Patent Use | Private Use | Disclose Source | License and Copyright Notice | Network Use is Distribution | Same License | State Changes | Liability | Trademark Use | Warranty |
|:----------------:|:--------------:|:------------:|:------------:|:----------:|:-----------:|:---------------:|:----------------------------:|:---------------------------:|:------------:|:-------------:|:---------:|:-------------:|:--------:|
| Apache-2.0       | 🟢             | 🟢           | 🟢           | 🟢         | 🟢          |                 | 🔵                           |                             |              | 🔵            | 🔴        | 🔴            | 🔴       |
| BSD-3-Clause     | 🟢             | 🟢           | 🟢           |            | 🟢          |                 | 🔵                           |                             |              |               | 🔴        |               | 🔴       |
| MIT              | 🟢             | 🟢           | 🟢           |            | 🟢          |                 | 🔵                           |                             |              |               | 🔴        |               | 🔴       |
| MulanPSL-2.0     | 🟢             | 🟢           | 🟢           | 🟢         | 🟢          |                 | 🔵                           |                             |              |               | 🔴        | 🔴            | 🔴       |

The drafter of the MulanPSL-2.0 license addressed similar concerns in this [comment](https://github.com/originjs/vite-plugin-federation/issues/464#issuecomment-1774859600):

> Thank you for raising this issue. Please allow me to explain. (I'm the one responsible for drafting MulanPSL-2.0 and getting it approved by OSI.)
>
> Actually at the beginning we just say in the license, english and chinese version have the same legal effect (because we carefully translated the two versions word by word, sentence by sentence). However, the OSI community suggested that IN CASE, in case there is a conflict between the two languages, we should indicate which language prevails.
>
> However, I must say, there is a tiny chance (close to zero) that this circumstance will happen. On the one hand, many people (including technical experts and lawyers) did careful proofreading between english version and chinese version; on the other hand, MulanPSL-2.0 is such a loose license that really doesn't have constrains, what conflict will you expect? We worry about conflict because we worry about legal risk that may bring, but since the legal terms are so loose we hardly see a risk.
</details>
