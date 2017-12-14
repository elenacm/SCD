.SUFFIXES:
.PHONY:    pc,pc2,pcm,fp,f,fi,fc,clean

compilador := mpicxx -std=c++11
flagsc     := -g -Wall

pc: prodcons_exe
	mpirun -np  3 ./$<

#PRODCONS CON UN PRODUCTOR Y UN CONSUMIDOR
pc2: prodcons2_exe
	mpirun -np  3 ./$<

#PRODCONS CON VARIOS PRODUCTORES Y CONSUMIDORES
pcm: prodcons2-mu_exe
	mpirun -np  10 ./$<

#PRODCONS LIFO CON VARIOS PRODUCTORES Y CONSUMIDORES
pcml: prodcons-mu-lifo_exe
	mpirun -np  10 ./$<

#PRODCONS FIFO CON VARIOS PRODUCTORES Y CONSUMIDORES
pcmf: prodcons-mu-fifo_exe
	mpirun -np  10 ./$<

fp: filosofos-plantilla_exe
	mpirun -np 10 ./$<

#FILÓSOFOS CON INTERBLOQUEO
fi: filosofos-interb_exe
	mpirun -np 10 ./$<

#FILÓSOFOS DE OTRA PERSONA QUE SI FUNCIONA
fa: filosofos-otro2_exe
	mpirun -np 11 ./$<

f: filosofos_exe
	mpirun -np 10 ./$<

#FILÓSOFOS CON UN CAMARERO
fc: filosofos-cam_exe
	mpirun -np 11 ./$<

%_exe: %.cpp
	$(compilador) $(flagsc) -o $@ $<

clean:
	rm -rf *_exe *.dSYM
