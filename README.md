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

See the documentation for more details.