# DbgScript

DbgScript is a multi-script debugger extension.

More precisely, it is a DbgEng extension that allows the embedding of arbitrary
scripting languages into the Debugging Tools for Windows.

DbgScript lets you harness the power of Ruby, Python or Lua to dump complex data
structures, mine data from dumps, conduct automated analyses, and more.

It also offers an extensibility model allowing new languages to be added with no
changes to the core.

DbgScript extends scripting languages with a API that models elements in the
target process like threads, C/C++ objects, stack frames, etc.

For example, in the Python Script Provider, a thread is modelled as a Thread
object with various attributes and methods that can obtain further information
about it, such as its call stack.

DbgScript comes with three first-party providers: Python, Ruby and Lua.

See the [documentation](http://alexbudmsft.github.io/dbgscript-docs) for more details.

**NOTE**: This debugger extension is in no way affiliated with the debugger team
(WinDbg, cdb, etc) and should be considered a 3rd party extension for all
intents and purposes.

## Versioning

This section describes version numbering scheme that DbgScript uses for releases.

Beta versions will be numbered X.Y.Z where Z > 0. (X and Y are the major/minor
numbers, respectively.)

Stable releases will have Z = 0.
 
E.g.
 
 * 1.0.0 -> First stable release.
 * 1.0.1 -> First beta
 * 1.0.2 -> Second beta
 * 1.0.N -> N'th beta.
 * 1.1.0 -> Stable release with major = 1, minor = 1.
 
Breaking changes will bump the major number.
