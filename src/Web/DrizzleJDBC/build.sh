
#  mvn clean dependency:copy-dependencies package
# mvn post-clean
# mvn [options] [<goal(s)>] [<phase(s)>]
#                            prepare-resources / compile / package / install

# mvn compile
# mvn clean
# mvn clean package



/bin/cp -f ../../Jaguar.java src/main/java/com/jaguar/jdbc/internal/jaguar

mvn -e -Dmaven.test.skip=true -Dmaven.javadoc.skip=true  package
