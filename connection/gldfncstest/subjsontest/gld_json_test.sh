#!/bin/bash

#export PATH="$PATH:/home/fncsdemo/fncs2-install/bin:/home/fncsdemo/gridlabd-master-install/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games"

echo $PATH

#export PATH

fncs_broker 2  &

#cd ~/FNCS-GLD-TESTCASE/gldfncs/fncs/tracer

#fncs_tracer 12s the_trace_file_output.out &


#cd ~/FNCS-GLD-TESTCASE/gldfncs/gld/

#gridlabd  IEEE_Dynamic_2gen_GLD3_nogen_jsonpub.glm > glmpub.out &

fncs_player_anon 12s fncsplayerout.txt &

#gridlabd IEEE_Dynamic_2gen_GLD3_nogen_jsonsub.glm &

gridlabd IEEE_13nodes_regulator_jsonsub.glm &


