for i in $(seq 1 $(nproc --all));
do
    nice -19 taskset -c $[i - 1] ./bin/Halogen-native "setoption name SyzygyPath value $1" "datagen output $i duration $2" > $i.log &
done