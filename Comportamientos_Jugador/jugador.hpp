#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"
using namespace std;

struct state{
  int row;
  int col;
  Orientacion compass;
};

class ComportamientoJugador : public Comportamiento{
  private:
  // Declarar aquí las variables de estado
  
  // Booleanos true si conocemos nuestra posicion, tenemos
  // zapato y bikini respectivamente
  bool position_known, shoes_on, bikini_on;
  // Estado actual
  state curr_state;
  // Última acción
  Action last_action;
  // Dimensiones del mapa
  int map_length, map_width;
  // Mapa auxiliar para anotar lo explorado mientras se desconoce la posición actual
  vector<vector<unsigned char>> aux_map;  
  // Posiciones de boosters guardados durante todas las vidas
  vector<pair<int,int>> shoes_locations, bikini_locations;

  // Inicialización del agente, útil para el constructor y respawn
  void initialize() {
    position_known = false;
    shoes_on = false;
    bikini_on = false;
    map_width = mapaResultado.size();
    map_length = map_width > 0 ? mapaResultado[0].size() : 0;
    aux_map.resize(2*map_width-1, vector<unsigned char>(2*map_length-1));
    curr_state.col = map_length-1;  
    curr_state.row = map_width-1;
    curr_state.compass = norte;
    last_action = actIDLE;    
    // Rellenar matrix auxiliar
    for (auto & row : aux_map) {
      for (auto & el : row) {
        el = '?';
      }
    }
  }

  // Función auxiliar que actualiza el mapa auxiliar o el mapa resultado
  // (dependiendo de si conocemos nuestra posición) con información sobre
  // el terreno que vemos en el momento dado
  void updateMap(vector<vector<unsigned char>> & v, Sensores sens);

  // Función auxiliar que inserta posición si noestá en el vector de posiciones
  void addPosToVector(vector<pair<int,int>> & v, pair<int,int> p);

  // Función auxiliar que transfiere el contenido del mapa auxiliar al mapa
  // resultado tras haber encontrado la posición del agente
  void transferToAnswerMap(Sensores sens);
  
  // Función auxiliar que calcula las coordenadas de la celda '?' sin explorar más cercana 
  pair<int,int> nearestUnexploredCell();

  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size){
      // Constructor de la clase
      // Dar el valor inicial a las variables de estado
      initialize();
    }

    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    int interact(Action accion, int valor);
};

#endif
