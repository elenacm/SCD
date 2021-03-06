/*
g++ -std=c++11 -pthread -Wfatal-errors -I. -o
barbero barbero1.cpp HoareMonitor.cpp
*/

#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <iomanip>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM;

const int num_barberos = 3, num_clientes = 6, tam_sala = 5;
mutex m;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio(){
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
//----------------------------------------------------------------------

class MBarberoSU : public HoareMonitor{
   private:
   CondVar barbero, sala_de_espera, silla_de_pelar;

   public:
   MBarberoSU();
   void siguienteCliente();
   void finCliente();
   void cortarPelo(int cliente);
 };
// -----------------------------------------------------------------------------

MBarberoSU::MBarberoSU(){
   sala_de_espera = newCondVar();
   barbero = newCondVar();
   silla_de_pelar = newCondVar();
}

void MBarberoSU::siguienteCliente(){
  if(sala_de_espera.get_nwt() == 0){
    m.lock();
    cout << "No hay clientes. El barbero se duerme." << endl;
    m.unlock();
    barbero.wait();
    m.lock();
    cout << "Barbero: Buenos días!!" << endl;
    m.unlock();
  }
  else{
    m.lock();
    cout << "Barbero: Que pase el siguiente cliente." << endl;
    m.unlock();
    sala_de_espera.signal();
  }
}

void MBarberoSU::finCliente(){
  m.lock();
  cout << "Barbero: Ya se puede marchar." << endl;
  m.unlock();
  silla_de_pelar.signal();
}

void MBarberoSU::cortarPelo(int cliente){
  if(barbero.get_nwt() != 0){
    m.lock();
    cout << "Cliente " << cliente << " despierta a barbero." << endl;
    m.unlock();
    barbero.signal();
  }
  else{
    if(sala_de_espera.get_nwt() >= tam_sala){
      m.lock();
      cout << "Cliente " << cliente <<": Hay mucha cola, luego vuelvo." << endl;
      m.unlock();
      return;
    }
    m.lock();
    cout << "Cliente " << cliente << ": Entro en la sala de espera." << endl;
    m.unlock();
    sala_de_espera.wait();
  }

  m.lock();
  cout << "Cliente " << cliente << " pelandose. " << endl;
  m.unlock();
  silla_de_pelar.wait();
  m.lock();
  cout << "Cliente " << cliente << ": Perfecto, hasta luego." << endl;
  m.unlock();
}

//----------------------------------------------------------------------

void esperarFueraBarberia(int cliente){
  chrono::milliseconds duracion_vuelta(aleatorio<100,200>());

  m.lock();
  cout << "                                " << cliente << " Me doy una vuelta." << endl;
  m.unlock();

  this_thread::sleep_for(duracion_vuelta);

  m.lock();
  cout << "                                " << cliente << " Vuelve a la barberia." << endl;
  m.unlock();

}

void cortarPeloACliente(){
  chrono::milliseconds duracion_pelado(aleatorio<20,100>());

  m.lock();
  cout << "Barbero pelando." << endl;
  m.unlock();

  this_thread::sleep_for(duracion_pelado);

  m.lock();
  cout << "Pelado terminado." << endl;
  m.unlock();

}

void funcion_hebra_barbero(MRef<MBarberoSU> monitor){

  while(true){
    //paso 1
    monitor->siguienteCliente();
    //paso 2
    cortarPeloACliente();
    //paso 3
    monitor->finCliente();
  }
}

//-------------------------------------------------------------------------

void  funcion_hebra_cliente(MRef<MBarberoSU> monitor, int num_cliente){
  cout << "Cliente " << num_cliente << " entra en la barberia." << endl;

  while(true){
    //pasos 1 y 2
    monitor->cortarPelo(num_cliente);
    //paso 3
    esperarFueraBarberia(num_cliente);
  }
}
//----------------------------------------------------------------------

int main(){
  // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
  MRef<MBarberoSU> barberia = Create<MBarberoSU>();

  // crear y lanzar hebras
   thread hebra_barbero(funcion_hebra_barbero, barberia),
          hebra_clientes[num_clientes];

    for(int i=0; i < num_clientes; i++)
      hebra_clientes[i]=thread(funcion_hebra_cliente, barberia, i);

    hebra_barbero.join();

    for(int i=0; i < num_clientes; i++)
      hebra_clientes[i].join();

    cout << "FIN" << endl;

    return 0;
}
