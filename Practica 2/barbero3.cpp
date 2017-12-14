#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include "HoareMonitor.hpp"

using namespace std ;
using namespace HM ;

mutex           mensaje;
const int       num_clientes=3;

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// *****************************************************************************
// clase para monitor Barrera, version 2,  semántica SU

class PeluqueriaSU : public HoareMonitor
{
   private:


   CondVar  CCortar,
            CEsperar,
            CDormir ;

   public:
   PeluqueriaSU() ; // constructor
   void cortarPelo( int num_hebra );
   void siguienteCliente();
   void finCliente();
} ;
// -----------------------------------------------------------------------------

PeluqueriaSU::PeluqueriaSU()
{
   CCortar            = newCondVar();
   CEsperar           = newCondVar();
   CDormir            = newCondVar();

}

void PeluqueriaSU::cortarPelo(int num_hebra){

  //Si el barbero está durmiendo y hay alguien en la sala de espera, lo despierta.
  if(!CDormir.empty() && !CEsperar.empty()){
    mensaje.lock();
    cout << "Cliente "<< num_hebra << " despierta al peluquero. " << endl;
    mensaje.unlock();
    CDormir.signal();
  }

  /*
  Cuando un cliente entra en la barbería debe pasar por la sala de espera
  independientemente de que el barbero esté durmiendo o pelando.
  */
    mensaje.lock();
    cout << "Cliente "<< num_hebra << " espera su turno." << endl;
    mensaje.unlock();
    CEsperar.wait();

  //Ha llegado su turno y espera mientras le cortan el pelo.
  mensaje.lock();
  cout << "Cliente "<< num_hebra << " se corta el pelo." << endl;
  mensaje.unlock();
  CCortar.wait();
  mensaje.lock();
  cout << "Cliente "<< num_hebra << " se va de la peluqueria" << endl;
  mensaje.unlock();
}
/*

*/
void PeluqueriaSU::siguienteCliente(){
  //Si no hay nadie esperando ni cortandose el pelo se duerme.
  if(CEsperar.empty() && CCortar.empty()){
    mensaje.lock();
    cout <<" El peluquero se duerme." << endl;
    mensaje.unlock();
    CDormir.wait();
  }
  //Permite que pase el cliente que haya en la cola en caso de que haya alguno.
  if(!CEsperar.empty()){
    mensaje.lock();
    cout << "El peluquero llama al siguiente cliente." << endl;
    mensaje.unlock();
    CEsperar.signal();
  }

}
/*

*/
void PeluqueriaSU::finCliente(){
  //Avisa de que ha terminado de cortarle el pelo.
  mensaje.lock();
  cout << "El peluquero termina de cortar el pelo." << endl;
  mensaje.unlock();
  CCortar.signal();
}
//----------------------------------------------------------------------
/*

*/
void cortarPeloAlCliente(){
  chrono::milliseconds produccion(aleatorio<20,200>());
  this_thread::sleep_for(produccion);
}
void funcion_hebra_peluquero( MRef<PeluqueriaSU> barberia)
{
  int ingrediente;
    while (true) {
      //Paso 1
      barberia->siguienteCliente();
      //Paso 2
      cortarPeloAlCliente();
      //Paso 3
      barberia->finCliente();
    }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void EsperarFueraBarberia( int num_cliente)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds vida_diaria( aleatorio<20,2000>() );


   mensaje.lock();
    cout << "Cliente " << num_cliente << "  :"
          << " sale de la peluqueria (" << vida_diaria.count() << " milisegundos)" << endl;
   mensaje.unlock();
   // espera bloqueada un tiempo igual a ''vida_diaria' milisegundos
   this_thread::sleep_for( vida_diaria );
  mensaje.lock();
    cout << "Cliente " << num_cliente << "  : entra a la peluqueria." << endl;
  mensaje.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del cliente
void  funcion_hebra_cliente( MRef<PeluqueriaSU> barberia, int num_cliente )
{
  int ingrediente;
   while( true ){
     //Pasos 1 y 2
     barberia->cortarPelo(num_cliente);
     //Paso 3
     EsperarFueraBarberia(num_cliente);
   }
}

//----------------------------------------------------------------------

int main()
{
  auto monitor = Create<PeluqueriaSU>();
    thread  peluquero,
            clientes[num_clientes];
    peluquero=thread(funcion_hebra_peluquero,monitor);
    for(int i=0; i < num_clientes; i++)
      clientes[i]=thread(funcion_hebra_cliente,monitor, i);

    peluquero.join();
    for(int i=0; i < num_clientes; i++)
      clientes[i].join();
}
