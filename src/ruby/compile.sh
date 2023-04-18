
#JAGINC="$HOME/src"
JAGINC=".."
RUBYINC="/usr/local/include/ruby-2.7.0"
RICEINC="/usr/local/lib/ruby/gems/2.7.0/gems/rice-2.2.0"
RUBYINC2="/usr/local/include/ruby-2.7.0/x86_64-linux"

OPT="-g"
OPT="-O3"

g++ $OPT -fPIC -I${JAGINC} -I${RUBYINC}  -I${RUBYINC2} -I${RICEINC} -c jaguarrb.cc

g++ -shared -fPIC \
 -L/usr/loca/lib \
 -L$RICEINC \
 -L$HOME/src/java \
 -o jaguarrb.so jaguarrb.o \
 -lJaguarClient -lruby $RICEINC/rice/librice.a -ldl -lz -luuid -lpthread 
 #-lJaguarClient -lruby -lrice -ldl -lz -luuid -lpthread 

export RUBYLIB=$RUBYLIB:.
 #-L/usr/local/lib/ruby/gems/2.3.0/gems/rice-2.1.0/rice \
