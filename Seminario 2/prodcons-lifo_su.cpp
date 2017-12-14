#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.hpp"

using namespace std;
using namespace HM;

constexpr int
   num_items  = 40 ,     // número de items a producir/consumir
   tam_vector = 10;
mutex
   mtx ;                 // mutex de escritura en pantalla
unsigned
   cont_prod[num_items], // contadores de verificación: producidos
   cont_cons[num_items]; // contadores de verificación: consumidos

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

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int n_hebra)
{
   static int contador = 0 ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "producido: " << contador << endl << flush ;
   mtx.unlock();
   cont_prod[contador] ++ ;
   return contador++ ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, int index )
{
   if ( num_items <= dato )
   {
      cout << " dato == " << dato << ", num_items == " << num_items << endl ;
      assert( dato < num_items );
   }
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   mtx.lock();
   cout << "               " << index << "   consumido: " << dato << endl ;
   mtx.unlock();
}
//----------------------------------------------------------------------

void ini_contadores()
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  cont_prod[i] = 0 ;
      cont_cons[i] = 0 ;
   }
}

//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." << flush ;

   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      if ( cont_prod[i] != 1 )
      {
         cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {
         cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

// *****************************************************************************
// clase para monitor Barrera, version LIFO, semántica SU, un prod. y un cons.

class MBarreraParSU : public HoareMonitor{
  private:
    int     cont,             // contador de hebras en cita
            num_hebras_cita;  // número total de hebras en cita
    CondVar cola;             // cola de hebras esperando en cita

  public:
    MBarreraParSU(int p_num_hebras_cita);   // constructor
    void cita(int num_hebra);               // método de cita
};
//------------------------------------------------------------------------------

MBarreraParSU::MBarreraParSU(int p_num_hebras_cita){
  num_hebras_cita = p_num_hebras_cita;
  cont = 0;
  cola = newCondVar();
}
//------------------------------------------------------------------------------

void MBarreraParSU::cita(int num_hebra){
  cont++;
  const int orden = cont;

  cout << "Llega hebra " << setw(2) << num_hebra << " (" << setw(2) << orden << ")." << endl;
  if(cont < num_hebras_cita)
    cola.wait();
  else{
    for(int i = 0; i < num_hebras_cita-1; i++)
      cola.signal();
    cont = 0;
  }
  cout << "     Sale hebra " << setw(2) << num_hebra << " (" << setw(2) << orden << " )." << endl;
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora( MRef<MBarreraParSU> monitor, int index )
{
   while(true){
     const int ms = aleatorio<0,30>();
     this_thread::sleep_for(chrono::milliseconds(ms));
     monitor->cita(index);
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora( MRef<MBarreraParSU> monitor, int index )
{
  while(true){
    const int ms = aleatorio<0,30>();
    this_thread::sleep_for(chrono::milliseconds(ms));
    monitor->cita(index);
  }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "-------------------------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (n prod/cons, Monitor SC, buffer LIFO). " << endl
        << "-------------------------------------------------------------------------------" << endl
        << flush ;

   cout <<  "Barrera parcial SU: inicio simulación." << endl ;

   // declarar el número total de hebras
   const int num_hebras      = 1000,
             num_hebras_cita = 10 ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<MBarreraParSU> monitor = Create<MBarreraParSU>( num_hebras_cita );

   // crear y lanzar hebras
   thread hebra_productora[num_hebras];
   thread hebra_consumidora[num_hebras];

   for( unsigned i = 0 ; i < num_hebras ; i++ )
      hebra_productora[i] = thread( funcion_hebra_productora, monitor, i );

   for(unsigned i = 0; i < num_hebras; i++)
      hebra_consumidora[i] = thread( funcion_hebra_consumidora, monitor, i);

   // esperar a que terminen las hebras (no pasa nunca)
   for( unsigned i = 0 ; i < num_hebras ; i++ )
      hebra_productora[i].join();

   for( unsigned i = 0 ; i < num_hebras ; i++ )
      hebra_consumidora[i].join();
}
