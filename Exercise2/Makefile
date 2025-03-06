bplus1:
	@echo " Compile bplus_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bp_main.c ./src/record.c ./src/bp_file.c ./src/bp_datanode.c ./src/bp_indexnode.c -lbf -o ./build/bplus_main -O2

clean1:
	@echo " Cleaning up build files...";
	rm -f ./build/bplus_main
	rm -f ./data.db

bplus2:
	@echo " Compile bplus_main ...";
	gcc -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/bp_main_2.c ./src/record.c ./src/bp_file.c ./src/bp_datanode.c ./src/bp_indexnode.c -lbf -o ./build/bplus_main -O2

clean2:
	@echo " Cleaning up build files...";
	rm -f ./build/bplus_main_2
	rm -f ./data.db