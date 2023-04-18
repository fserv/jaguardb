
#echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH ..."
JAR=$HOME/DrizzleJDBC-2.0/target/jaguar-jdbc-2.0.jar

javac -cp $JAR DataSourceBill.java
java -Djava.library.path=$HOME/raydb.git -cp $JAR:. DataSourceBill

