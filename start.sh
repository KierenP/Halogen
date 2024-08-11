for i in $(seq 1 $(nproc --all));
do
    nice -19 taskset -c $[i - 1] ./bin/Halogen-pgo "setoption name SyzygyPath value <PATH>" "datagen output $i.data duration $1" > $i.log &
done