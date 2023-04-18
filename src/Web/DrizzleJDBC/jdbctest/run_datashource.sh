
LIBDIR=$HOME/jaguar/lib
JAR=$LIBDIR/jaguar-jdbc-2.0.jar

export LD_LIBRARY_PATH=$LIBDIR

javac -cp $JAR DataSourceTest.java

port=`grep PORT $HOME/jaguar/conf/server.conf |grep -v '#' | awk -F= '{print $2}'`
java -Djava.library.path=$LIBDIR -cp $JAR:. DataSourceTest $port

