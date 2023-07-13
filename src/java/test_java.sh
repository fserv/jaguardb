
JAGUAR_HOME=$HOME/jaguar
LIBPATH=$JAGUAR_HOME/lib
export LD_LIBRARY_PATH=$LIBPATH

/bin/rm -f *.class

javac -cp $LIBPATH/jaguar-jdbc-2.0.jar:.  example.java
java -Djava.library.path=$LIBPATH -cp $LIBPATH/jaguar-jdbc-2.0.jar:. example 192.168.1.88 8888

javac -cp $LIBPATH/jaguar-jdbc-2.0.jar:.  JaguarJDBCTest.java
java -Djava.library.path=$LIBPATH -cp $LIBPATH/jaguar-jdbc-2.0.jar:. JaguarJDBCTest  192.168.1.88 8888

