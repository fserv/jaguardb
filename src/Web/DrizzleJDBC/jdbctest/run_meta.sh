
JAR=$HOME/DrizzleJDBC//target/jaguar-jdbc-2.0.jar

javac -cp $JAR  MetadataTest.java
java -Djava.library.path=$HOME/raydb.git -cp $JAR:. MetadataTest
