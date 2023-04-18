export LD_LIBRARY_PATH=$HOME/jaguar/lib:/home/jaguar/jaguar/lib:/usr/local/gcc-7.1.0/lib64
unset GOPATH
export GO111MODULE=on

go run main.go 9999
