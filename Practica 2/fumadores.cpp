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

const int tam_vec = 3;


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
//----------------------------------------------------------------------


class MFumadorSU : public HoareMonitor
{
   private:
   CondVar estanquero;
   int mostrador;
   CondVar fumador[tam_vec];

   public:
   MFumadorSU();            // constructor
   void obtenerIngrediente(int ingrediente);   // método para obtener un ingrediente
   void ponerIngrediente(int ingrediente);     // método para poner un ingrediente
   void esperarRecogidaIngrediente();      // método de espera de recogida de un ingrediente
};
// -----------------------------------------------------------------------------

MFumadorSU::MFumadorSU(){
   mostrador = -1;

   for(int i = 0; i < tam_vec; i++)
    fumador[i] = newCondVar();

   estanquero = newCondVar();
}

// -----------------------------------------------------------------------------

void MFumadorSU::obtenerIngrediente(int ingrediente){
  if(mostrador != ingrediente){
    cout << "                       Fumador esperando ingredientes." << endl;
    fumador[ingrediente].wait();
  }

  mostrador = -1;
  estanquero.signal();
}

// -----------------------------------------------------------------------------

void MFumadorSU::ponerIngrediente(int ingrediente){
  mostrador = ingrediente;
  cout << "Ingrediente " << mostrador << " en el mostrador." << endl;
  fumador[ingrediente].signal();
}

// -----------------------------------------------------------------------------

void MFumadorSU::esperarRecogidaIngrediente(){

  if(mostrador != -1){
    cout << "                         Esperando recogida de ingredientes." << endl;
    estanquero.wait();
  }

}

//----------------------------------------------------------------------
// función que ejecuta la hebra del estanquero
int producir(){
  chrono::milliseconds duracion_producir( aleatorio<20,200>() );

  int ingrediente = aleatorio <0,2>();
  cout << "Se ha producido ingrediente " << ingrediente << endl;
  this_thread::sleep_for( duracion_producir );
  return ingrediente;
}

void funcion_hebra_estanquero(MRef<MFumadorSU> monitor){
  int dato;
  while(true){
    dato = producir();
    monitor->ponerIngrediente(dato);
    monitor->esperarRecogidaIngrediente();
  }
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar(int num_fumador){
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>());

   // informa de que comienza a fumar
    cout << "Fumador " << num_fumador << ": "
          << "empieza a fumar " << endl;
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    cout << "Fumador " << num_fumador << ": termina de fumar, comienza espera de ingrediente." << endl;
}

//----------------------------------------------------------------------
// función que ejecuta la hebra del fumador
void  funcion_hebra_fumador(MRef<MFumadorSU> monitor, int num_fumador){
   while( true ){
     monitor->obtenerIngrediente(num_fumador);
     fumar(num_fumador);
   }
}

//----------------------------------------------------------------------

int main()
{
  // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
  MRef<MFumadorSU> Estanco = Create<MFumadorSU>();

  // crear y lanzar hebras
   thread hebra_estanquero(funcion_hebra_estanquero, Estanco),
          hebra_fumador[tam_vec];

    for(int i=0; i < tam_vec; i++)
      hebra_fumador[i]=thread(funcion_hebra_fumador, Estanco, i);

    hebra_estanquero.join();

    for(int i=0; i < tam_vec; i++)
      hebra_fumador[i].join();

    cout << "FIN" << endl;

    return 0;
}
