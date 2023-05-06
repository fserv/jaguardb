
for n in `cut -d: -f1 ServerCfg6.txt`
do
    echo "$n ..."
    ssh $n "cd ~/consistent_hash_db; /bin/rm -rf mytestdb/*"
done

