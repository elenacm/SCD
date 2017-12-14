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
const int       num_fumadores=3;




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

class EstancoSU : public HoareMonitor
{
   private:
   int      mostrador;            // contador de hebras en cita

   CondVar  CEst,
            CFum[num_fumadores] ;            // cola de hebras esperando en cita

   public:
   EstancoSU() ; // constructor
   void ObtenerIngrediente( int num_hebra );
   void PonerIngrediente(int num_hebra);
   void EsperarRecogida();
} ;
// -----------------------------------------------------------------------------

EstancoSU::EstancoSU()
{
   mostrador       = -1 ;
   CEst            = newCondVar();
   for(int i=0; i<num_fumadores; i++){
     CFum[i]=newCondVar();
   }
}

void EstancoSU::ObtenerIngrediente(int num_hebra){
  if(mostrador!=num_hebra) CFum[num_hebra].wait();
  mostrador=-1;
  CEst.signal();
}

void EstancoSU::PonerIngrediente(int num_hebra){
  mostrador=num_hebra;
  CFum[num_hebra].signal();
}

void EstancoSU::EsperarRecogida(){
  if(mostrador!=-1) CEst.wait();
}
//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
int producir(){
  //chrono::milliseconds produccion(aleatorio<20,200>());
  //this_thread::sleep_for(produccion);
  return aleatorio<0,num_fumadores-1>();
}
void funcion_hebra_estanquero( MRef<EstancoSU> monitor)
{
  int ingrediente;
    while (true) {
      ingrediente=producir(); //Si le ponemos tiempo de espera sería después de sem_wait
      monitor->PonerIngrediente(ingrediente);
      mensaje.lock();
      std::cout << "\t\tPuesto ingrediente: "<< ingrediente << endl;
      mensaje.unlock();
      monitor->EsperarRecogida();
    }

}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
   mensaje.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
   mensaje.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
  mensaje.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
  mensaje.unlock();
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador( MRef<EstancoSU> monitor, int num_fumador )
{
  int ingrediente;
   while( true ){
     monitor->ObtenerIngrediente(num_fumador);
     mensaje.lock();
     cout << "\t\tRetirado ingrediente: " << num_fumador << endl;
     mensaje.unlock();
     fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
  auto monitor = Create<EstancoSU>();
    thread  estanquero,
            fumadores[num_fumadores];
    estanquero=thread(funcion_hebra_estanquero,monitor);
    for(int i=0; i < num_fumadores; i++)
      fumadores[i]=thread(funcion_hebra_fumador,monitor, i);

    estanquero.join();
    for(int i=0; i < num_fumadores; i++)
      fumadores[i].join();
}
