#export LD_LIBRARY_PATH=$HOME/jaguar/lib:/usr/local/gcc-7.1.0/lib64
export LD_LIBRARY_PATH=$HOME/jaguar/lib:/lib/gcc/x86_64-linux-gnu/11
unset GOPATH
export GO111MODULE=on

go run main.go 8888
