if not exist build (
	md build
)

pushd build
cmake -G "Visual Studio 14 2015 Win64" ..
popd

if not exist build_rb_prov (
	md build_rb_prov
)

pushd build_rb_prov
cmake -G "Visual Studio 12 2013 Win64" ..\src\rubyprov
popd