@echo off
pushd "%~dp0"
bin\I386\NT\main -MLWpass . -load images\I386\NT\batch.img . %*
popd