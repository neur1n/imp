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
- macOS: Pinyin - Simplified, Wubi - Simplified, or WeType (微信输入法), paired
  with ABC.
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
  macOS SDK; ABC and the native source for your selected pair installed and
  enabled.
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

This installs `bin/imp` and, if it does not already exist, `bin/imp.conf`.
Reinstalling does not overwrite an existing configuration.

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

On macOS, select an input-source pair with `g:imp_pair`:

```vim
let g:imp_pair = 'pinyin'
```

Or in Lua:

```lua
vim.g.imp_pair = "pinyin"
```

Pair names come from the configuration file; the preset includes `wetype`,
`pinyin`, and `wubi`. Leaving `g:imp_pair` unset or empty uses the first pair.
The plugin passes the selected pair to both `status` and `set` commands.

### Pair File
On macOS, imp reads `imp.conf` beside the executable, not in the current working
directory. The repository's [`bin/imp.conf`](bin/imp.conf) contains:

```text
# name alphanumeric_source_id native_source_id
wetype com.apple.keylayout.ABC com.tencent.inputmethod.wetype.pinyin
pinyin com.apple.keylayout.ABC com.apple.inputmethod.SCIM.ITABC
wubi com.apple.keylayout.ABC com.apple.inputmethod.SCIM.WBX
```

The first record is the default. Add, remove, reorder, or edit pairs without
rebuilding. A file replaces the built-in list; it is not merged with it.
Records must have exactly three whitespace-separated fields and unique,
case-sensitive names. Empty lines and full-line `#` comments are ignored;
inline comments, quotes, and escapes are not supported. Use source IDs rather
than display names. To find an ID, select the source in macOS and run `imp status`.

Use an explicit file, for example one outside the plugin directory that will
not be affected by plugin updates:

```vim
let g:imp_config = '/absolute/path/to/imp.conf'
```

```lua
vim.g.imp_config = "/absolute/path/to/imp.conf"
```

Unset or empty `g:imp_config` uses automatic discovery. When using `build/imp`
for development, automatic discovery looks in `build/`, not `bin/`; set
`g:imp_config` to the repository's `bin/imp.conf` if desired.

Each CLI invocation rereads the file. If no automatic file exists, imp uses
the built-in pairs with `wetype` first. An explicit file must exist; unreadable,
empty, malformed, and duplicate-name configurations produce errors rather
than falling back. Other platforms do not support explicit pair configuration.

After editing a file while the plugin is enabled, call `imp#resync()` to reread
and apply it; file changes are not watched. Changing `g:imp_pair` or
`g:imp_config` is noticed on the next synchronization. Already queued requests
retain their pair name and config path, but read the file's contents when
their CLI process starts.

## CLI
Use the executable directly, or add its directory to `PATH`:

```sh
imp status
imp set native
imp set alphanumeric
imp --help
imp --version
```

Choose a macOS pair with `--pair <name>`, before or after the command:

```sh
imp --pair pinyin status
imp --pair pinyin set native
imp --pair pinyin set alphanumeric
imp --pair wubi set native
```

The choice applies only to that invocation; pass the same pair on each call.
Without `--pair`, macOS uses the first pair in its loaded configuration.
Override the automatically discovered file with `--config <path>`:

```sh
imp --config /absolute/path/to/imp.conf status
imp --config /absolute/path/to/imp.conf --pair pinyin set native
```

Both options may appear before or after the command. Relative `--config` paths
are resolved against the CLI's working directory; executable-relative discovery
resolves symlinks to the actual executable. `--help` and `--version` do not load
configuration.

`status` prints `key=value` lines: `backend`, `mode`, and, when available,
`source_id` and `source_name`. `set` prints the requested mode, such as
`mode=native`, on success. `mode=unknown` means the current state could not be
mapped to the selected pair; it is not itself an error.

Success exits with `0`; failures exit with `EXIT_FAILURE` and print `error:
[domain:code] message` to `stderr`, without a success result on `stdout`.

## Platform Support
### macOS
The `macos-tis` backend switches between configured input-source pairs using
TIS. All preset pairs use ABC (`com.apple.keylayout.ABC`) for `alphanumeric`:

| Pair | Native input source | Native source ID |
| --- | --- | --- |
| `wetype` (default) | WeType (微信输入法) | `com.tencent.inputmethod.wetype.pinyin` |
| `pinyin` | Pinyin - Simplified | `com.apple.inputmethod.SCIM.ITABC` |
| `wubi` | Wubi - Simplified | `com.apple.inputmethod.SCIM.WBX` |

This changes the selected source, not the input method's internal conversion
mode; selecting WeType does not guarantee Chinese mode. No keyboard events are
simulated.

The selected pair determines both target sources and status interpretation,
regardless of the current source. A source outside that pair reports
`mode=unknown`. Unknown pair names cause errors; there is no fallback to another
pair. The file may include sources that are not installed or enabled; loading
and querying do not require them to be available.

Only the target of a `set` command must be installed, enabled, and selectable.
Errors distinguish sources not found among installed sources, disabled sources,
and sources that cannot be selected, and include the target source ID. A missing
native source does not prevent switching to an available alphanumeric source.
imp never automatically installs or enables sources.

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
