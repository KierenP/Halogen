for i in $(seq 1 $(nproc --all));
do
    nice -19 taskset -c $[i - 1] ./Halogen-pgo "setoption name SyzygyPath value <PATH>" "datagen $i.data" > $i.log &
done