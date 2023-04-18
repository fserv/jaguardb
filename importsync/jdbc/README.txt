
Java program to export data from any database that supports JDBC

Compiling:
===================================================================

1) install ant if not already.
	Steps to install ant to your home directory. you may use a different version of ant. 
	a. download apache-ant-1.10.1-bin.tar.gz to your home directory
	b. run tar -xvzf apache-ant-1.10.1-bin.tar.gz 
	c. add export ANT_HOME=~/apache-ant-1.10.1 to .bashrc
	d. add export PATH=$PATH:$ANT_HOME/bin to .bashrc
	e. run . .bashrc


2) run ant
   ./compile.sh


Testing:
===================================================================

0) assume the table is already created with correct columns in the target database. The order of 
columns in the table is same as in the source database.

1) create/update a property file 'app.conf'

2) run: 

    java -cp lib/mysql-connector-java-5.1.43-bin.jar:lib/jaguar-jdbc-2.0.jar:lib/jdbcsql.jar -Dapp.conf=app.conf com.jaguar.jdbcsql.Importer

