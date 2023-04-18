
JAR=$HOME/DrizzleJDBC/target/jaguar-jdbc-2.0.jar
echo "JAR is $JAR "

export LD_LIBRARY_PATH=$HOME/jaguar/lib:/usr/local/gcc-4.9.3/lib64

javac -cp $JAR DriverTest.java
java -Djava.library.path=$HOME/jaguar/lib -cp $JAR:.  DriverTest
