/*

*/
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
const int       num_clientes=10,
                sillas_disponibles=5,
                cantidad_peluqueros=2;


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

  const int numero_sillas,
            numero_peluqueros;
        int num_ocupadas;

   CondVar  CCortar,
            CEsperar,
            CDormir ,
            CPuerta ;

   public:
   PeluqueriaSU(int sillas, int peluqueros) ; // constructor
   void cortarPelo( int num_hebra );
   void siguienteCliente(int num_hebra);
   void finCliente(int num_hebra);
} ;
// -----------------------------------------------------------------------------

PeluqueriaSU::PeluqueriaSU(int sillas, int peluqueros)
:numero_sillas(sillas), numero_peluqueros(peluqueros)
{
  num_ocupadas        = 0;
   CCortar            = newCondVar();
   CEsperar           = newCondVar();
   CDormir            = newCondVar();
   CPuerta            = newCondVar();

}

void PeluqueriaSU::cortarPelo(int num_hebra){
  if(!CDormir.empty()){
    mensaje.lock();
    cout << "Cliente "<< num_hebra << " despierta al peluquero. " << endl;
    mensaje.unlock();
    CDormir.signal();
  }

  //Espera en la cola hasta que llegue su turno en caso de que haya alguien delante

  if(!CCortar.empty() || !CEsperar.empty()){
    //La primera condicion es para tratar de impedir que se cuele alguien antes
    //de que pase el primero que estaba esperando.
    if(!CPuerta.empty() || num_ocupadas >= numero_sillas){
      mensaje.lock();
      cout << "Cliente "<< num_hebra << " espera en la puerta de la barberia." << endl;
      mensaje.unlock();
      CPuerta.wait();
    }
    num_ocupadas++; //Ocupamos una silla.
    mensaje.lock();
    cout << "Cliente "<< num_hebra << " espera su turno. Sillas Ocupadas: "<< num_ocupadas << endl;
    mensaje.unlock();
    CEsperar.wait();
    num_ocupadas--; //Desocupamos una silla.
    //Si hay alguien esperando en la puerta le indicamos que puede pasar.
    if(!CPuerta.empty())
      CPuerta.signal();

  }

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
  En esta version, o se duermen los dos o no se duerme ninguno.
  Habria que revisarlo de cara al examen.
*/
void PeluqueriaSU::siguienteCliente(int num_hebra){
  //Si no hay nadie esperando ni cortandose el pelo se duerme.
  if(CEsperar.empty() && CCortar.empty()){
    mensaje.lock();
    cout <<" El peluquero "<< num_hebra <<" se duerme." << endl;
    mensaje.unlock();
    CDormir.wait();
  }
  //Permite que pase el cliente que haya en la cola en caso de que haya alguno.
  if(!CEsperar.empty()){
    mensaje.lock();
    cout << "El peluquero " << num_hebra << " llama al siguiente cliente." << endl;
    mensaje.unlock();
    CEsperar.signal();
  }

}
/*

*/
void PeluqueriaSU::finCliente(int num_hebra){
  //Avisa de que ha terminado de cortarle el pelo.
  mensaje.lock();
  cout << "El peluquero " << num_hebra << " termina de cortar el pelo." << endl;
  mensaje.unlock();
  CCortar.signal();
}
//----------------------------------------------------------------------
/*

*/
void cortarPeloAlCliente(){
  chrono::milliseconds produccion(aleatorio<20,100>());
  this_thread::sleep_for(produccion);
}


void funcion_hebra_peluquero( MRef<PeluqueriaSU> barberia, int num_hebra)
{
  int ingrediente;
    while (true) {
      //Paso 1
      barberia->siguienteCliente(num_hebra);
      //Paso 2
      cortarPeloAlCliente();
      //Paso 3
      barberia->finCliente(num_hebra);
    }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void EsperarFueraBarberia( int num_cliente)
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds vida_diaria( aleatorio<100,500>() );

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
  auto monitor = Create<PeluqueriaSU>(sillas_disponibles,cantidad_peluqueros);
    thread  peluquero[cantidad_peluqueros],
            clientes[num_clientes];
    for(int i=0; i< cantidad_peluqueros; i++)
      peluquero[i]=thread(funcion_hebra_peluquero,monitor,i);
    for(int i=0; i < num_clientes; i++)
      clientes[i]=thread(funcion_hebra_cliente,monitor, i);

    for(int i=0; i< cantidad_peluqueros; i++)
      peluquero[i].join();
    for(int i=0; i < num_clientes; i++)
      clientes[i].join();
}
