#include "../Comportamientos_Jugador/jugador.hpp"
#include <iostream>
#include <algorithm>
using namespace std;


Action ComportamientoJugador::think(Sensores sensores){

	Action accion = actIDLE;

	// Actualizamos estado en función de sensores e última acción
	
	if (sensores.reset) {	// Agente muerto, respawn en punto random, inicializamos
		initialize();
	} else if (!sensores.colision) {	// Agente no choca, última acción exitosa
										// Si choca se quedara en bucle colisionando
		// Procesamos acción
		if (last_action == actFORWARD) {
			switch (curr_state.compass) {
				case norte: --curr_state.row; break;
				case noreste: --curr_state.row; ++curr_state.col; break;
				case este: ++curr_state.col; break;
				case sureste: ++curr_state.row; ++curr_state.col; break;
				case sur: ++curr_state.row; break;
				case suroeste: ++curr_state.row; --curr_state.col; break;
				case oeste: --curr_state.col; break;
				case noroeste: --curr_state.row; --curr_state.col; break;			
			}
		} else if (last_action != actIDLE){
			int temp = curr_state.compass;
			switch (last_action) {
			case actTURN_SL:
				temp = (temp+7)%8;	// Mismo que a-1+8
				break;
			case actTURN_SR:
				temp = (temp+1)%8;
				break;
			case actTURN_BL:
				temp = (temp+5)%8;	// Mismo que a-3+8
				break;
			case actTURN_BR:
				temp = (temp+3)%8;
				break;
			}
			curr_state.compass = static_cast<Orientacion>(temp);
		}
	}

	// Actualizamos estado si encontramos booster
	// En caso de estar posicionado, añadimos la posición del booster
	// al vector que almacena sus posiciones para futuras vidas

	switch(sensores.terreno[0]) {
		// Si encontramos posición
		case 'G':
			if (!position_known) {
				// Transferimos lo explorado al resultado
				transferToAnswerMap(sensores);
				// Pasamos de coordenadas del mapa auxiliar a las
				// del mapa resultado
				curr_state.row = sensores.posF;
				curr_state.col = sensores.posC;
				position_known = true;
			}
			break;
		// Si encontramos zapatos
		case 'D':
			shoes_on = true;
			if (position_known) {
				addPosToVector(shoes_locations, {curr_state.row, curr_state.col});
			}
			break;
		// Si encontramos bikini
		case 'K':
			bikini_on = true;
			if (position_known) {
				addPosToVector(bikini_locations, {curr_state.row, curr_state.col});
			}
			break;
	}

	// QUITAR!!!!!!!
	curr_state.row = sensores.posF;
	curr_state.col = sensores.posC;
	curr_state.compass = sensores.sentido;
	position_known = true;

	// Actualizamos matriz
	updateMap(position_known ? mapaResultado : aux_map, sensores);
	
	// Decidimos nueva acción
	accion = actFORWARD;

	// Actualizamos última acción
	last_action = accion;

	// Retornamos nuestra siguiente acción
	return accion;
}

int ComportamientoJugador::interact(Action accion, int valor){
  return false;
}

void ComportamientoJugador::updateMap(vector<vector<unsigned char>> & v, Sensores sens) {
	Orientacion ori = curr_state.compass;
	int count = 0, sign = 0, r = 0, c = 0;
	// Visión triangular
	if (ori == norte || ori == sur || ori == este || ori == oeste) {
		sign = (ori == norte || ori == este) ? -1 : 1;
		for (int i = 0; i <= 3; ++i) {
			for (int j = -i; j <= i; ++j) {
				r = curr_state.row + sign*(ori == norte || ori == sur ? i : j);
				c = curr_state.col - sign*(ori == norte || ori == sur ? j : i);
				v[r][c] = sens.terreno[count++];
			}
		}
	// Visión cuadrada
	} else {
		// Genero vista hacia noroeste y hago las rotaciones apropiadas
		int rot_row = 0, rot_col = 0;
		for (int i = 0; i <= 3 ; ++i) {
			c = curr_state.col - i;
			r = curr_state.row + 1;
			for (int j = 0; j <= 2*i; ++j) {
				if (j <= i) {
					--r;
				} else {
					++c;
				}
				
				switch (ori) {
					case noroeste:
						rot_row = r;
						rot_col = c;
						break;
					case noreste:
						rot_row = curr_state.row-curr_state.col+c;
						rot_col = curr_state.row+curr_state.col-r;
						break;
					case sureste:
						rot_row = 2*curr_state.row-r;
						rot_col = 2*curr_state.col-c;
						break;
					case suroeste:
						rot_row = curr_state.row+curr_state.col-c;
						rot_col = curr_state.col-curr_state.row+r;
						break;
				}
				v[rot_row][rot_col] = sens.terreno[count++];
			}
		}
	}
}

void ComportamientoJugador::addPosToVector(vector<pair<int,int>> & v, pair<int,int> p) {
	for (auto & el : v) {
		if (el == p) {
			return;
		}
	}
	v.push_back(p);
}

void ComportamientoJugador::transferToAnswerMap(Sensores sens) {
	int r = curr_state.row-sens.posF, c = curr_state.col-sens.posC;
	unsigned char cell;
	for (int i = 0; i < map_width; ++i) {
		for (int j = 0; j < map_length; ++j) {
			// Rellenamos mapa resultado
			mapaResultado[i][j] = aux_map[r+i][c+j];
			cell = mapaResultado[i][j];
			// Añadimos boosters encontrados
			if (cell == 'D') {
				addPosToVector(shoes_locations, {i,j});
			} else if (cell == 'K') {
				addPosToVector(bikini_locations, {i,j});
			}
		}
	}
}

pair<int,int> ComportamientoJugador::nearestUnexploredCell() {
	pair<int,int> answer;
	// Dimensiones del mapa
	int rows = position_known ? map_width : 2*map_width - 1;
	int cols = position_known ? map_length : 2*map_length - 1;
	// Respuesta por defecto
	answer.first = position_known ? 0 : map_width-1;
	answer.second = position_known ? 0 : map_length-1;
	
	// Booleana que es false al final de iteración si 
	// no quedan celdas por comprobar
	bool unchecked = true;
	int level = 0, pos_r = curr_state.row, pos_c = curr_state.col;
	int lbc = 0, ubc = 0, lbr = 0, ubr = 0;
	unsigned char c;

	// Mientras quedan elementos por comprobar
	while (unchecked) {
		unchecked = false;
		++level;
		lbc = pos_c - level >= 0 ? pos_c - level : 0;
		ubc = pos_c + level < cols ? pos_c + level : cols - 1;
		lbr = pos_r - level >= 0 ? pos_r - level : 0;
		ubr = pos_r + level < rows ? pos_r + level : rows - 1;

		// Norte
		if (pos_r - level >= 0) {
			unchecked = true;
			for (int i = lbc; i <= ubc; ++i) {
				c = position_known ? mapaResultado[pos_r-level][i] : aux_map[pos_r-level][i];
				if (c == '?') {
					return {pos_r-level, i};
				}
			}
		}
		// Sur
		if (pos_r + level < rows) {
			unchecked = true;
			for (int i = lbc; i <= ubc; ++i) {
				c = position_known ? mapaResultado[pos_r+level][i] : aux_map[pos_r+level][i];
				if (c == '?') {
					return {pos_r+level, i};
				}
			}
		}
		// Este
		if (pos_c + level < cols) {
			unchecked = true;
			for (int i = lbr; i <= ubr; ++i) {
				c = position_known ? mapaResultado[i][pos_c+level] : aux_map[i][pos_c+level];
				if (c == '?') {
					return {i, pos_c+level};
				}
			}
		}
		// Oeste
		if (pos_c - level >= 0) {
			unchecked = true;
			for (int i = lbr; i <= ubr; ++i) {
				c = position_known ? mapaResultado[i][pos_c-level] : aux_map[i][pos_c-level];
				if (c == '?') {
					return {i, pos_c-level};
				}
			}
		}
	}

	return answer;
}
