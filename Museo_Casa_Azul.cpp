// ==================================================================================== =
// PROYECTO: MUSEO CASA AZUL (FRIDA KAHLO)
// ARCHIVO:  Museo_Casa_Azul.cpp
//
// DESCRIPCIÓN:
// Aplicación interactiva en 3D que simula un recorrido por el Museo Casa Azul.
// Utiliza OpenGL para el renderizado, GLFW para la ventana y entradas, y GLM
// para las operaciones matemáticas. Incluye carga de modelos estáticos y animados,
// audio de fondo, un skybox, y un sistema de animación por keyframes.
// =====================================================================================


//-------------------------------------------------------------------------------------
// 1. INCLUSIÓN DE LIBRERÍAS
//-------------------------------------------------------------------------------------
#include <Windows.h>					// API de Windows (necesaria para mmsystem.h)
#include <glad/glad.h>					// Cargador de funciones de OpenGL (debe ir antes de GLFW)
#include <glfw3.h>						// Para crear ventanas, manejar contexto OpenGL y entradas (teclado/ratón)
#include <stdlib.h>						// Librería estándar de C (utilidades generales)
#include <glm/glm.hpp>					// Librería principal de matemáticas de OpenGL (vectores, matrices)
#include <glm/gtc/matrix_transform.hpp>	// Para transformaciones (translate, rotate, scale, perspective)
#include <glm/gtc/type_ptr.hpp>			// Para convertir tipos de GLM a punteros compatibles con OpenGL
#include <time.h>						// Para manejo de tiempo (usado en la semilla aleatoria)

// --- Librerías de terceros (incluidas en el proyecto) ---

// Define STB_IMAGE_IMPLEMENTATION una sola vez para incluir el cuerpo de la función
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>					// Librería para cargar texturas (imágenes) desde archivos

// Define SDL_MAIN_HANDLED para evitar conflictos con la función main()
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>					// Librería SDL3 (usada aquí para control de tiempo preciso - SDL_GetTicks)

// Define MINIAUDIO_IMPLEMENTATION una sola vez
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"					// Librería ligera para reproducción de audio

//-------------------------------------------------------------------------------------
// 2. INCLUSIÓN DE CABECERAS PERSONALIZADAS
//-------------------------------------------------------------------------------------
#include <shader_m.h>					// Clase personalizada para compilar y manejar Shaders
#include <camera.h>						// Clase personalizada para manejar la cámara (posición, vista, movimiento)
#include <modelAnim.h>					// Clase para cargar y renderizar modelos animados (ej. .dae de Mixamo)
#include <model.h>						// Clase para cargar y renderizar modelos estáticos (ej. .obj)
#include <Skybox.h>						// Clase para renderizar el entorno (cielo/fondo)
#include <iostream>						// Para entrada/salida en consola (std::cout)
#include <mmsystem.h>					// Librería multimedia de Windows (complementa a Windows.h)
#include <vector>						// Para manejar arreglos dinámicos (usado para el enjambre de mariposas)
#include <ctime>						// Para manejo de tiempo (complementa a time.h)

//-------------------------------------------------------------------------------------
// 3. DECLARACIÓN DE FUNCIONES (PROTOTIPOS)
//-------------------------------------------------------------------------------------
// Callbacks de GLFW (se ejecutan cuando ocurre un evento)
void framebuffer_size_callback(GLFWwindow* window, int width, int height); // Se llama al cambiar el tamaño de la ventana
void mouse_callback(GLFWwindow* window, double xpos, double ypos);         // Se llama al mover el ratón
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);  // Se llama al usar la rueda del ratón
void my_input(GLFWwindow* window, int key, int scancode, int action, int mods); // Se llama al presionar/soltar una tecla

// Funciones de control y utilitarias
void animate(void);                                                       // Actualiza todas las variables de animación por frame
void getResolution(void);                                                 // Obtiene la resolución del monitor principal
void myData(void);                                                        // Configura VAOs/VBOs para geometrías primitivas (piso, lienzo)
void LoadTextures(void);                                                  // Carga las texturas desde archivos
unsigned int generateTextures(char*, bool, bool);

//-------------------------------------------------------------------------------------
// 4. VARIABLES GLOBALES Y CONFIGURACIÓN
//-------------------------------------------------------------------------------------

// --- Configuración de Ventana y Pantalla ---
unsigned int SCR_WIDTH = 800;	// Ancho inicial de la ventana (se sobrescribe con la resolución)
unsigned int SCR_HEIGHT = 600;	// Alto inicial de la ventana (se sobrescribe con la resolución)
GLFWmonitor* monitors;			// Puntero al monitor principal

// --- Buffers de OpenGL para Geometrías Primitivas ---
// VAO (Vertex Array Object): Almacena la configuración de un objeto
// VBO (Vertex Buffer Object): Almacena los datos de los vértices
// EBO (Element Buffer Object): Almacena los índices de los vértices
GLuint VBO[3], VAO[3], EBO[3];	// Arreglos para manejar los buffers (0=cuadro, 1=cubo, 2=piso)

// --- Configuración de Cámara ---
Camera camera(glm::vec3(0.0f, 500.0f, -5000.0f)); // Posición inicial (elevada, -5000 en Z)
float MovementSpeed = 0.8f;						// Velocidad de movimiento (factor)
GLfloat lastX = SCR_WIDTH / 2.0f;				// Posición X inicial del ratón (centro)
GLfloat lastY = SCR_HEIGHT / 2.0f;				// Posición Y inicial del ratón (centro)
bool firstMouse = true;							// Bandera para evitar saltos al inicio

// --- Control de Tiempo y FPS ---
const int FPS = 60;					// Cuadros por segundo deseados
const int LOOP_TIME = 1000 / FPS;	// Tiempo por bucle (en milisegundos)
double deltaTime = 0.0f;			// Tiempo entre el cuadro actual y el anterior
double lastFrame = 0.0f;			// Tiempo del último cuadro registrado

// --- Variables de Teclado (No usadas actualmente para movimiento) ---
float movX = 0.0f, movY = 0.0f, movZ = -5.0f;
float rotX = 0.0f;

// --- Identificadores de Texturas (Handles de OpenGL) ---
unsigned int t_rojo, t_rosa, t_naranja, t_azul, t_verde, t_piedra; // Texturas para pinturas y piso

// --- Configuración de Iluminación Global (Luz Direccional) ---
glm::vec3 lightPosition(0.0f, 4.0f, -10.0f);	// Posición (usada por luces puntuales)
glm::vec3 lightDirection(0.0f, -1.0f, -1.0f);	// Dirección de la luz (simula el sol)
glm::vec3 lightColor = glm::vec3(0.7f);			// Color base
glm::vec3 diffuseColor = lightColor * glm::vec3(0.5f); // Color difuso
glm::vec3 ambientColor = diffuseColor * glm::vec3(0.75f); // Color ambiental

// --- Variables de Animación General ---
bool animacion = false;		// Activa/desactiva la animación (No usada directamente, se usa 'play')

// Variables de rotación (animación independiente)
float rotSilla = 0.0f;		// Rotación actual de la silla mecedora
float rotRodIzq = 0.0f;		// Rotación del rodillo izquierdo (no usado)

// Incrementos (no usados en la animación principal)
float rotSillaInc = 0.0f;
float rotRodIzqInc = 0.0f;

// --- Variables de Animación: CABALLETE (Keyframes) ---
// Posición general del modelo compuesto
float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

// Posiciones (offset Y) individuales de cada parte (para desensamble)
float basePosY = 0.0f;
float pataIzqPosY = 0.0f;
float pataDerPosY = 0.0f;
float pataTrasPosY = 0.0f;
float soporteTrasPosY = 0.0f;
float adornoPosY = 0.0f;

// Rotaciones individuales de cada parte
float baseRot = 0.0f;
float pataIzqRot = 0.0f;
float pataDerRot = 0.0f;
float pataTrasRot = 0.0f;
float soporteTrasRot = 0.0f;
float adornoRot = 0.0f;
float pinturaRotZ = 0.0f; // Rotación Z (eje frontal) de la pintura

// --- Incrementos de Animación: CABALLETE (Calculados por interpolación) ---
// Estos valores se calculan en `interpolation()` y se aplican en `animate()`
float incX = 0.0f, incY = 0.0f, incZ = 0.0f; // Incrementos de posición general

// Incrementos de posición (offset Y)
float basePosYInc = 0.0f;
float pataIzqPosYInc = 0.0f;
float pataDerPosYInc = 0.0f;
float pataTrasPosYInc = 0.0f;
float soporteTrasPosYInc = 0.0f;
float adornoPosYInc = 0.0f;

// Incrementos de rotación
float baseRotInc = 0.0f;
float pataIzqRotInc = 0.0f;
float pataDerRotInc = 0.0f;
float pataTrasRotInc = 0.0f;
float soporteTrasRotInc = 0.0f;
float adornoRotInc = 0.0f;
float pinturaRotZInc = 0.0f;

// ------------------------------------------------------------------------------------ -
// 5. SISTEMA DE KEYFRAMES (Animación Principal - Caballete)
//-------------------------------------------------------------------------------------

#define MAX_FRAMES 20			// Número máximo de fotogramas clave que se pueden guardar
int i_max_steps = 45;			// Pasos de interpolación entre dos keyframes (más alto = más lento/suave)
int i_curr_steps = 0;			// Paso actual en la interpolación

/**
 * @struct FRAME
 * @brief Almacena el estado completo (posición y rotación) de todas las
 * piezas del caballete en un instante (fotograma clave) específico.
 */
typedef struct _frame
{
	// Variables no usadas (heredadas)
	float rotSilla;
	float rotRodIzq;

	// Posición general del caballete
	float posX, posY, posZ;

	// Posición (Y) y Rotación de cada pieza
	float basePosY, baseRot;
	float pataIzqPosY, pataIzqRot;
	float pataDerPosY, pataDerRot;
	float pataTrasPosY, pataTrasRot;
	float soporteTrasPosY, soporteTrasRot;
	float pinturaPosY, pinturaRot, pinturaRotZ;
	float adornoPosY, adornoRot;

} FRAME;

// --- Almacenamiento y Control de Keyframes ---
FRAME KeyFrame[MAX_FRAMES];		// Arreglo global que almacena la secuencia de animación
int FrameIndex = 20;			// Índice actual de frame (se usa para *guardar*, no para *definir*)
bool play = false;				// Bandera: ¿Se está reproduciendo la animación?
int playIndex = 0;				// Índice del keyframe actual durante la reproducción

/**
 * @brief Guarda el estado actual de todas las variables globales
 * en un nuevo fotograma clave (KeyFrame).
 * @note Esta función no se usa en el código actual, pero permite
 * crear animaciones dinámicamente (ej. presionando una tecla).
 */
void saveFrame(void)
{
	std::cout << "Frame Index = " << FrameIndex << std::endl;

	KeyFrame[FrameIndex].posX = posX;
	KeyFrame[FrameIndex].posY = posY;
	KeyFrame[FrameIndex].posZ = posZ;

	KeyFrame[FrameIndex].basePosY = basePosY;
	KeyFrame[FrameIndex].pataIzqPosY = pataIzqPosY;
	KeyFrame[FrameIndex].pataDerPosY = pataDerPosY;
	KeyFrame[FrameIndex].pataTrasPosY = pataTrasPosY;
	KeyFrame[FrameIndex].soporteTrasPosY = soporteTrasPosY;
	KeyFrame[FrameIndex].adornoPosY = adornoPosY;

	KeyFrame[FrameIndex].baseRot = baseRot;
	KeyFrame[FrameIndex].pataIzqRot = pataIzqRot;
	KeyFrame[FrameIndex].pataDerRot = pataDerRot;
	KeyFrame[FrameIndex].pataTrasRot = pataTrasRot;
	KeyFrame[FrameIndex].soporteTrasRot = soporteTrasRot;
	KeyFrame[FrameIndex].adornoRot = adornoRot;
	KeyFrame[FrameIndex].pinturaRotZ = pinturaRotZ;

	FrameIndex++;
}

/**
 * @brief Restaura todas las variables globales de animación al estado
 * guardado en el KeyFrame[0] (el estado inicial).
 */
void resetElements(void)
{
	// Restaura las posiciones del caballete
	posX = KeyFrame[0].posX;
	posY = KeyFrame[0].posY;
	posZ = KeyFrame[0].posZ;

	// Posiciones individuales
	basePosY = KeyFrame[0].basePosY;
	pataIzqPosY = KeyFrame[0].pataIzqPosY;
	pataDerPosY = KeyFrame[0].pataDerPosY;
	pataTrasPosY = KeyFrame[0].pataTrasPosY;
	soporteTrasPosY = KeyFrame[0].soporteTrasPosY;
	adornoPosY = KeyFrame[0].adornoPosY;

	// Rotaciones individuales
	baseRot = KeyFrame[0].baseRot;
	pataIzqRot = KeyFrame[0].pataIzqRot;
	pataDerRot = KeyFrame[0].pataDerRot;
	pataTrasRot = KeyFrame[0].pataTrasRot;
	soporteTrasRot = KeyFrame[0].soporteTrasRot;
	adornoRot = KeyFrame[0].adornoRot;
	pinturaRotZ = KeyFrame[0].pinturaRotZ;

	// Reinicia los índices de reproducción
	playIndex = 0;
	i_curr_steps = 0;
}

/**
 * @brief Calcula los incrementos (deltas) necesarios para moverse suavemente
 * desde el KeyFrame[playIndex] actual hasta el KeyFrame[playIndex + 1].
 * Los resultados se guardan en las variables globales '...Inc'.
 */
void interpolation(void)
{
	// Incrementos para la posición general
	incX = (KeyFrame[playIndex + 1].posX - KeyFrame[playIndex].posX) / i_max_steps;
	incY = (KeyFrame[playIndex + 1].posY - KeyFrame[playIndex].posY) / i_max_steps;
	incZ = (KeyFrame[playIndex + 1].posZ - KeyFrame[playIndex].posZ) / i_max_steps;

	// Incrementos para las posiciones verticales (desensamble)
	basePosYInc = (KeyFrame[playIndex + 1].basePosY - KeyFrame[playIndex].basePosY) / float(i_max_steps);
	pataIzqPosYInc = (KeyFrame[playIndex + 1].pataIzqPosY - KeyFrame[playIndex].pataIzqPosY) / float(i_max_steps);
	pataDerPosYInc = (KeyFrame[playIndex + 1].pataDerPosY - KeyFrame[playIndex].pataDerPosY) / float(i_max_steps);
	pataTrasPosYInc = (KeyFrame[playIndex + 1].pataTrasPosY - KeyFrame[playIndex].pataTrasPosY) / float(i_max_steps);
	soporteTrasPosYInc = (KeyFrame[playIndex + 1].soporteTrasPosY - KeyFrame[playIndex].soporteTrasPosY) / float(i_max_steps);
	adornoPosYInc = (KeyFrame[playIndex + 1].adornoPosY - KeyFrame[playIndex].adornoPosY) / float(i_max_steps);

	// Incrementos para las rotaciones
	baseRotInc = (KeyFrame[playIndex + 1].baseRot - KeyFrame[playIndex].baseRot) / float(i_max_steps);
	pataIzqRotInc = (KeyFrame[playIndex + 1].pataIzqRot - KeyFrame[playIndex].pataIzqRot) / float(i_max_steps);
	pataDerRotInc = (KeyFrame[playIndex + 1].pataDerRot - KeyFrame[playIndex].pataDerRot) / float(i_max_steps);
	pataTrasRotInc = (KeyFrame[playIndex + 1].pataTrasRot - KeyFrame[playIndex].pataTrasRot) / float(i_max_steps);
	soporteTrasRotInc = (KeyFrame[playIndex + 1].soporteTrasRot - KeyFrame[playIndex].soporteTrasRot) / float(i_max_steps);
	adornoRotInc = (KeyFrame[playIndex + 1].adornoRot - KeyFrame[playIndex].adornoRot) / float(i_max_steps);
	pinturaRotZInc = (KeyFrame[playIndex + 1].pinturaRotZ - KeyFrame[playIndex].pinturaRotZ) / float(i_max_steps);
}

//-------------------------------------------------------------------------------------
// 6. SISTEMA DE ANIMACIÓN (Silla Mecedora)
//-------------------------------------------------------------------------------------
#define MAX_FRAMES_SILLA 4		// Número de keyframes para la silla

/**
 * @struct SILLAFRAME
 * @brief Almacena el estado (rotación) de la silla mecedora en un keyframe.
 */
typedef struct _sillaFrame {
	float rotSilla; // Ángulo de rotación de la silla
} SILLAFRAME;

// Arreglo que almacena los keyframes de la silla
SILLAFRAME SillaKeyFrame[MAX_FRAMES_SILLA];

// --- Variables de control de la Silla Mecedora ---
int playIndexSilla = 0;			// Índice actual del keyframe en reproducción
int i_max_steps_silla = 45;		// Pasos de interpolación (duración)
bool playSilla = false;			// Bandera: ¿Se está animando la silla?
float sillaInterpT = 0.0f;		// Variable de interpolación (0.0 a 1.0)
bool firstPlaySilla = true;		// Bandera para reiniciar la animación

//-------------------------------------------------------------------------------------
// 7. SISTEMA DE SIMULACIÓN (Mariposas)
//-------------------------------------------------------------------------------------

/**
 * @struct Mariposa
 * @brief Representa una mariposa individual del enjambre.
 */
struct Mariposa {
	glm::vec3 posicionBase; // Posición central alrededor de la cual vuela
	float offsetX;
	float offsetY;
	float offsetZ;
	float velocidad;        // Velocidad de la oscilación
	float escala;           // Tamaño
	float fase;             // Desfase en la función sin/cos para que no se muevan igual
};

// Vector que almacena todas las mariposas
std::vector<Mariposa> enjambre;

/**
 * @brief Genera un conjunto de mariposas con parámetros aleatorios
 * alrededor de una posición base.
 */
void inicializarMariposas(int cantidad) {
	srand((unsigned int)time(NULL));		// Inicializa la semilla aleatoria
	glm::vec3 basePos(1000.0f, 500.0f, -2340.0f); // Posición central del enjambre

	for (int i = 0; i < cantidad; i++) {
		Mariposa m;
		m.posicionBase = basePos + glm::vec3(
			((rand() % 400) - 200),		// Desplazamiento X
			((rand() % 200) - 100),		// Desplazamiento Y
			((rand() % 400) - 200)		// Desplazamiento Z
		);
		m.velocidad = 0.5f + static_cast<float>(rand() % 100) / 200.0f;
		m.escala = 0.3f + static_cast<float>(rand() % 30) / 100.0f;
		m.fase = static_cast<float>(rand() % 360);

		enjambre.push_back(m); // Agrega la mariposa al vector
	}
}
//-------------------------------------------------------------------------------------
// 8. SISTEMA DE ANIMACIÓN (Pincel y Lienzo)
//-------------------------------------------------------------------------------------

#define MAX_FRAMES_PINCEL 5 // Keyframes para el movimiento del pincel

/**
 * @struct PINCELFRAME
 * @brief Almacena la posición y rotación del pincel en un keyframe.
 */
typedef struct _pincelFrame {
	float posX, posY, posZ;
	float rotZ;
} PINCELFRAME;

PINCELFRAME PincelKeyFrame[MAX_FRAMES_PINCEL]; // Arreglo de keyframes del pincel

// --- Variables de control del Pincel ---
int playIndexPincel = 0;          // Índice actual del keyframe
float pincelInterpT = 0.0f;       // Valor de interpolación (0.0 a 1.0)
bool playPincel = false;          // Bandera: ¿Se está animando el pincel?
bool firstPlayPincel = true;      // Bandera para reiniciar
int i_max_steps_pincel = 30;      // Pasos de interpolación

// Posición y rotación actual del pincel (actualizadas en animate())
float posPincelX = 2875.0f;
float posPincelY = 380.0f;
float posPincelZ = -1000.0f;
float rotPincelZ = 0.0f;

// --- Variables de control del Lienzo (Texturas) ---
unsigned int texturaPintura[5]; // Arreglo de texturas para las capas
int pinturaActual = 0;      // Índice de la textura activa (0 a 4)
float mezclaPintura = 0.0f; // Nivel de mezcla (no implementado en el shader)

//-------------------------------------------------------------------------------------
// 9. FUNCIONES UTILITARIAS (Carga de Texturas)
//-------------------------------------------------------------------------------------

/**
 * @brief Carga una imagen, genera una textura OpenGL y devuelve su ID.
 * @param filename Ruta del archivo de imagen.
 * @param alfa Indica si la imagen tiene canal alfa (transparencia).
 * @param isPrimitive Indica si debe voltear la imagen verticalmente (stb_image vs OpenGL).
 */
unsigned int generateTextures(const char* filename, bool alfa, bool isPrimitive)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Parámetros de texturizado
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height, nrChannels;

	// stbi_load carga las imágenes al revés (Y-flip)
	if (isPrimitive)
		stbi_set_flip_vertically_on_load(true); // Para primitivas (piso)
	else
		stbi_set_flip_vertically_on_load(false); // Para modelos (cuadros)

	unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		// Carga la textura con o sin canal alfa
		if (alfa)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

		glGenerateMipmap(GL_TEXTURE_2D);
		return textureID;
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
		return 100;
	}

	stbi_image_free(data); // Libera la memoria de la imagen cargada
}

/**
 * @brief Carga todas las texturas principales usadas en el lienzo y el piso.
 */
void LoadTextures()
{
	t_piedra = generateTextures("Texturas/piso.jpg", 0, true);
	t_rosa = generateTextures("Texturas/lupemarin.jpg", 0, false);
	t_rojo = generateTextures("Texturas/autoretrato_frame.jpg", 0, false);
	t_verde = generateTextures("Texturas/autorretrato_feo.jpg", 0, false);
	t_naranja = generateTextures("Texturas/yo_perro.jpg", 0, false);
	t_azul = generateTextures("Texturas/diego_frida.jpg", 0, false);
}

//-------------------------------------------------------------------------------------
// 10. FOCO DE LUZ (Spotlight)
//-------------------------------------------------------------------------------------

// Configuración de la luz focal (lámpara sobre el lienzo)
glm::vec3 focoPos = glm::vec3(2960.0f, 400.0f, -1500.0f); // Posición
glm::vec3 focoDir = glm::vec3(0.0f, -1.0f, 0.0f);     // Dirección (hacia abajo)
float focoIntensidad = 20.0f; // Intensidad (se anima)
bool focoSubiendo = true;   // Dirección de la animación de intensidad

//-------------------------------------------------------------------------------------
// 11. FUNCIÓN DE ANIMACIÓN PRINCIPAL (animate)
//-------------------------------------------------------------------------------------

/**
 * @brief Se llama en cada fotograma desde el bucle principal.
 * Actualiza todas las variables de estado para las animaciones
 * (Caballete, Silla, Pincel, Foco).
 */
void animate(void)
{
	// Factor de velocidad para compensar variaciones en deltaTime
	float speedFactor = 1.0f;
	if (deltaTime > 0.0) speedFactor = float(deltaTime * 80.0);
	speedFactor = glm::clamp(speedFactor, 0.1f, 5.0f); // Limita el factor

	// ==========================================================
	// 1. ANIMACIÓN DEL CABALLETE (Interpolación Lineal Simple)
	// ==========================================================
	if (play)
	{
		// Lógica de avance de pasos (no usada, se incrementa 1)
		// int stepAdvance = std::max(1, (int)std::round(speedFactor));

		// Si se completaron los pasos de interpolación...
		if (i_curr_steps >= i_max_steps)
		{
			playIndex++; // Avanza al siguiente keyframe

			// Si la animación terminó (llegó al penúltimo frame)...
			if (playIndex > FrameIndex - 2)
			{
				std::cout << "Animation ended" << std::endl;
				playIndex = 0; // Reinicia
				play = false;  // Detiene la reproducción
			}
			else
			{
				// Si hay más frames, calcula la interpolación para el siguiente tramo
				i_curr_steps = 0;
				interpolation();
			}
		}
		else
		{
			// Si aún está interpolando, aplica los incrementos calculados
			// (Se aplica speedFactor, aunque los incrementos ya están divididos por i_max_steps.
			// Esto puede hacer la animación dependiente de la tasa de fotogramas si no se maneja con cuidado)

			// Aplica incrementos de posición
			posX += incX * speedFactor;
			posY += incY * speedFactor;
			posZ += incZ * speedFactor;

			// Aplica incrementos de posición (desensamble)
			basePosY += basePosYInc * speedFactor;
			pataIzqPosY += pataIzqPosYInc * speedFactor;
			pataDerPosY += pataDerPosYInc * speedFactor;
			pataTrasPosY += pataTrasPosYInc * speedFactor;
			soporteTrasPosY += soporteTrasPosYInc * speedFactor;
			adornoPosY += adornoPosYInc * speedFactor;

			// Aplica incrementos de rotación
			baseRot += baseRotInc * speedFactor;
			pataIzqRot += pataIzqRotInc * speedFactor;
			pataDerRot += pataDerRotInc * speedFactor;
			pataTrasRot += pataTrasRotInc * speedFactor;
			soporteTrasRot += soporteTrasRotInc * speedFactor;
			adornoRot += adornoRotInc * speedFactor;
			pinturaRotZ += pinturaRotZInc * speedFactor;

			i_curr_steps++; // Avanza un paso de interpolación
		}
	}

	// ==========================================================
	// 2. ANIMACIÓN DEL FOCO (Oscilación de Intensidad)
	// ==========================================================
	if (focoSubiendo) {
		focoIntensidad += 0.02f * speedFactor;
		if (focoIntensidad >= 1.0f) focoSubiendo = false;
	}
	else {
		focoIntensidad -= 0.02f * speedFactor;
		if (focoIntensidad <= 0.0f) focoSubiendo = true;
	}

	// ==========================================================
	// 3. ANIMACIÓN DE LA SILLA MECEDORA (Interpolación LERP Cíclica)
	// ==========================================================
	if (playSilla) {
		if (firstPlaySilla) {
			playIndexSilla = 0;
			sillaInterpT = 0.0f;
			firstPlaySilla = false;
		}

		// Incremento de interpolación (T)
		float tInc = 1.0f / float(i_max_steps_silla);
		sillaInterpT += tInc * speedFactor;

		// Interpola entre el par de keyframes actual usando glm::mix (LERP)
		if (playIndexSilla == 0) { // Tramo 0 -> 1
			rotSilla = glm::mix(SillaKeyFrame[0].rotSilla, SillaKeyFrame[1].rotSilla, sillaInterpT);
			if (sillaInterpT >= 1.0f) { sillaInterpT = 0.0f; playIndexSilla = 1; }
		}
		else if (playIndexSilla == 1) { // Tramo 1 -> 2
			rotSilla = glm::mix(SillaKeyFrame[1].rotSilla, SillaKeyFrame[2].rotSilla, sillaInterpT);
			if (sillaInterpT >= 1.0f) { sillaInterpT = 0.0f; playIndexSilla = 2; }
		}
		else if (playIndexSilla == 2) { // Tramo 2 -> 3
			rotSilla = glm::mix(SillaKeyFrame[2].rotSilla, SillaKeyFrame[3].rotSilla, sillaInterpT);
			if (sillaInterpT >= 1.0f) { sillaInterpT = 0.0f; playIndexSilla = 3; }
		}
		else if (playIndexSilla == 3) { // Tramo 3 -> 0 (Cierra el ciclo)
			rotSilla = glm::mix(SillaKeyFrame[3].rotSilla, SillaKeyFrame[0].rotSilla, sillaInterpT);
			if (sillaInterpT >= 1.0f) { sillaInterpT = 0.0f; playIndexSilla = 0; }
		}
	}
	else {
		firstPlaySilla = true; // Prepara para el reinicio
	}

	// ==========================================================
	// 4. ANIMACIÓN DEL PINCEL (LERP Cíclica + Cambio de Textura)
	// ==========================================================
	if (playPincel) {
		if (firstPlayPincel) {
			playIndexPincel = 0;
			pincelInterpT = 0.0f;
			firstPlayPincel = false;
		}

		float tInc = 1.0f / float(i_max_steps_pincel);
		pincelInterpT += tInc * speedFactor;
		mezclaPintura = pincelInterpT; // (No usado en shader, pero actualiza)

		// Interpola posición y rotación del pincel
		if (playIndexPincel == 0) { // Tramo 0 -> 1
			posPincelX = glm::mix(PincelKeyFrame[0].posX, PincelKeyFrame[1].posX, pincelInterpT);
			posPincelY = glm::mix(PincelKeyFrame[0].posY, PincelKeyFrame[1].posY, pincelInterpT);
			posPincelZ = glm::mix(PincelKeyFrame[0].posZ, PincelKeyFrame[1].posZ, pincelInterpT);
			rotPincelZ = glm::mix(PincelKeyFrame[0].rotZ, PincelKeyFrame[1].rotZ, pincelInterpT);
			// Al terminar el tramo, cambia la textura del lienzo
			if (pincelInterpT >= 1.0f) { pincelInterpT = 0.0f; playIndexPincel = 1; pinturaActual = 1; }
		}
		else if (playIndexPincel == 1) { // Tramo 1 -> 2
			posPincelX = glm::mix(PincelKeyFrame[1].posX, PincelKeyFrame[2].posX, pincelInterpT);
			posPincelY = glm::mix(PincelKeyFrame[1].posY, PincelKeyFrame[2].posY, pincelInterpT);
			posPincelZ = glm::mix(PincelKeyFrame[1].posZ, PincelKeyFrame[2].posZ, pincelInterpT);
			rotPincelZ = glm::mix(PincelKeyFrame[1].rotZ, PincelKeyFrame[2].rotZ, pincelInterpT);
			if (pincelInterpT >= 1.0f) { pincelInterpT = 0.0f; playIndexPincel = 2; pinturaActual = 2; }
		}
		else if (playIndexPincel == 2) { // Tramo 2 -> 3
			posPincelX = glm::mix(PincelKeyFrame[2].posX, PincelKeyFrame[3].posX, pincelInterpT);
			posPincelY = glm::mix(PincelKeyFrame[2].posY, PincelKeyFrame[3].posY, pincelInterpT);
			posPincelZ = glm::mix(PincelKeyFrame[2].posZ, PincelKeyFrame[3].posZ, pincelInterpT);
			rotPincelZ = glm::mix(PincelKeyFrame[2].rotZ, PincelKeyFrame[3].rotZ, pincelInterpT);
			if (pincelInterpT >= 1.0f) { pincelInterpT = 0.0f; playIndexPincel = 3; pinturaActual = 3; }
		}
		else if (playIndexPincel == 3) { // Tramo 3 -> 4 (Regreso)
			posPincelX = glm::mix(PincelKeyFrame[3].posX, PincelKeyFrame[4].posX, pincelInterpT);
			posPincelY = glm::mix(PincelKeyFrame[3].posY, PincelKeyFrame[4].posY, pincelInterpT);
			posPincelZ = glm::mix(PincelKeyFrame[3].posZ, PincelKeyFrame[4].posZ, pincelInterpT);
			rotPincelZ = glm::mix(PincelKeyFrame[3].rotZ, PincelKeyFrame[4].rotZ, pincelInterpT);
			if (pincelInterpT >= 1.0f) { pincelInterpT = 0.0f; playIndexPincel = 0; pinturaActual = 4; } // Reinicia el ciclo
		}
	}
	else {
		firstPlayPincel = true; // Prepara para el reinicio
	}
}


//-------------------------------------------------------------------------------------
// 12. FUNCIONES DE CONFIGURACIÓN INICIAL
//-------------------------------------------------------------------------------------

/**
 * @brief Obtiene la resolución del monitor principal y ajusta las variables
 * globales SCR_WIDTH y SCR_HEIGHT.
 */
void getResolution() {
	const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	SCR_WIDTH = mode->width;
	SCR_HEIGHT = (mode->height) - 80; // Resta 80 píxeles (para la barra de tareas)
}

/**
 * @brief Define la geometría (vértices, texturas, índices) para los
 * objetos primitivos (Piso, Cuadro, Cubo) y los sube a la GPU
 * mediante VAOs, VBOs y EBOs.
 */
void myData() {

	//-----------------------------
	// 1. Vértices del Cuadro (Lienzo)
	//-----------------------------
	float vertices[] = {
		// posiciones (x,y,z) // coords textura (u,v)
		 0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // sup der
		 0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // inf der
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // inf izq
		-0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // sup izq
	};
	unsigned int indices[] = { 0, 1, 3, 1, 2, 3 }; // Dos triángulos

	//-----------------------------
	// 2. Vértices del Piso (Plano grande con repetición de textura)
	//-----------------------------
	float verticesPiso[] = {
		// posiciones          // coords textura (repetidas 40x)
		 100.0f,  100.0f, 0.0f,   40.0f, 40.0f,
		 100.0f, -100.0f, 0.0f,   40.0f,  0.0f,
		-100.0f, -100.0f, 0.0f,    0.0f,  0.0f,
		-100.0f,  100.0f, 0.0f,    0.0f, 40.0f
	};
	unsigned int indicesPiso[] = { 0, 1, 3, 1, 2, 3 };

	//-----------------------------
	// 3. Vértices del Cubo (No usado en la escena final)
	//-----------------------------
	GLfloat verticesCubo[] = {
		// (Datos del cubo omitidos por brevedad) ...
		-0.5f, -0.5f, 0.5f,  0.0f, 0.0f, 0.5f, -0.5f, 0.5f,  1.0f, 0.0f, // ...
	};

	//-----------------------------
	// Creación de VAO, VBO y EBO
	//-----------------------------
	glGenVertexArrays(3, VAO); // Genera 3 VAOs
	glGenBuffers(3, VBO);    // Genera 3 VBOs
	glGenBuffers(3, EBO);    // Genera 3 EBOs

	//==========================
	// 1. Configuración del Cuadro (Lienzo) -> VAO[0]
	//==========================
	glBindVertexArray(VAO[0]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	// Atributo 0: Posición (x, y, z)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Atributo 1: Coordenadas de textura (u, v)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//==========================
	// 2. Configuración del Piso -> VAO[2]
	//==========================
	glBindVertexArray(VAO[2]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesPiso), verticesPiso, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesPiso), indicesPiso, GL_STATIC_DRAW);
	// Atributo 0: Posición
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// Atributo 1: Coordenadas de textura
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//==========================
	// 3. Configuración del Cubo -> VAO[1] (No usado)
	//==========================
	glBindVertexArray(VAO[1]);
	glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCubo), verticesCubo, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Desvincula los buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

//-------------------------------------------------------------------------------------
// 13. FUNCIÓN PRINCIPAL (main)
//-------------------------------------------------------------------------------------
int main() {

	// =========================================================================
	// 1. INICIALIZACIÓN DE GLFW Y VENTANA
	// =========================================================================
	glfwInit();
	monitors = glfwGetPrimaryMonitor();
	getResolution(); // Obtiene el tamaño de pantalla

	// Creación de la ventana
	GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Museo Casa Azul", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetWindowPos(window, 0, 30); // Posiciona la ventana
	glfwMakeContextCurrent(window);

	// Vinculación de Callbacks
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetKeyCallback(window, my_input);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Muestra el cursor

	// =========================================================================
	// 2. CARGA DE FUNCIONES OPENGL (GLAD)
	// =========================================================================
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	// =========================================================================
	// 3. CONFIGURACIÓN GLOBAL DE OPENGL
	// =========================================================================
	LoadTextures();  // Carga texturas (piedra, pinturas)
	myData();        // Sube la geometría del piso y lienzo a la GPU
	glEnable(GL_DEPTH_TEST);  // Activa el test de profundidad (Z-buffer)

	// =========================================================================
	// 4. COMPILACIÓN Y CARGA DE SHADERS
	// =========================================================================
	Shader myShader("shaders/shader_texture_color.vs", "shaders/shader_texture_color.fs");	// Para primitivas (piso, lienzo)
	Shader staticShader("Shaders/shader_Lights.vs", "Shaders/shader_Lights_mod.fs");		// Para modelos 3D estáticos (con luces)
	Shader skyboxShader("Shaders/skybox.vs", "Shaders/skybox.fs");							// Para el skybox
	Shader animShader("Shaders/anim.vs", "Shaders/anim.fs");								// Para modelos 3D animados (Mixamo)

	// =========================================================================
	// 5. CONFIGURACIÓN DEL SKYBOX
	// =========================================================================
	vector<std::string> faces{ // Rutas de las 6 caras del skybox
		"resources/skybox/right-coyoacan.png",
		"resources/skybox/left-coyoacan.png",
		"resources/skybox/top-coyoacan.png",
		"resources/skybox/bottom-coyoacan.png",
		"resources/skybox/front-coyoacan.png",
		"resources/skybox/back-coyoacan.png"
	};
	Skybox skybox = Skybox(faces); // Crea el objeto Skybox
	skyboxShader.use();
	skyboxShader.setInt("skybox", 0); // Vincula el shader al sampler 0

	// =========================================================================
	// 6. CARGA DE MODELOS 3D
	// =========================================================================

	// --- Escenario Principal ---
	Model museo("resources/objects/Museo_Casa_Azul/museo_frida_kahlo.obj");

	// --- Colección de Pinturas (Agrupadas por carpetas) ---
	Model pintura_01("resources/objects/Arte/Pinturas01/autorretrato-con-pelo-corto.obj");
	Model pintura_02("resources/objects/Arte/Pinturas01/autorretrato-con-stalin.obj");
	Model pintura_03("resources/objects/Arte/Pinturas01/autorretrato-mono-plantas.obj");
	Model pintura_04("resources/objects/Arte/Pinturas01/autorretrato-pelo-rizado.obj");
	Model pintura_05("resources/objects/Arte/Pinturas01/columna-rota.obj");
	Model pintura_06("resources/objects/Arte/Pinturas01/dos-fridas.obj");
	Model pintura_07("resources/objects/Arte/Pinturas01/yo-y-mi-munieca.obj");

	Model pintura_08("resources/objects/Arte/Pinturas02/viva-la-vida.obj");
	Model pintura_09("resources/objects/Arte/Pinturas02/pintura-tunas.obj");
	Model pintura_10("resources/objects/Arte/Pinturas02/pintura-cocos.obj");
	Model pintura_11("resources/objects/Arte/Pinturas02/abuelos.obj");
	Model pintura_12("resources/objects/Arte/Pinturas02/mi-nacimiento.obj");
	Model pintura_13("resources/objects/Arte/Pinturas02/mascara-de-muerte.obj");
	Model pintura_14("resources/objects/Arte/Pinturas02/frida-y-diego.obj");

	Model pintura_15("resources/objects/Arte/Pinturas03/suicidio-dorothy-hale.obj");
	Model pintura_16("resources/objects/Arte/Pinturas03/memoria-el-corazon.obj");
	Model pintura_17("resources/objects/Arte/Pinturas03/yo-y-mis-pericos.obj");
	Model pintura_18("resources/objects/Arte/Pinturas03/luther-burbank.obj");
	Model pintura_19("resources/objects/Arte/Pinturas03/la-mascara.obj");
	Model pintura_20("resources/objects/Arte/Pinturas03/diego-y-yo.obj");
	Model pintura_21("resources/objects/Arte/Pinturas03/marxismo.obj");

	// --- Vitrinas ---
	Model vitrina_01("resources/objects/Vitrinas/Vitrina01.obj");
	Model vitrina_02("resources/objects/Vitrinas/Vitrina02.obj");
	Model vitrina_03("resources/objects/Vitrinas/Vitrina03.obj");

	// --- Mobiliario y Otros ---
	Model banca("resources/objects/Banca/banca.obj");
	Model silla_mecedora("resources/objects/Silla_Mecedora/silla-mecedora.obj");
	Model lampara("resources/objects/Lampara/lampara.obj");
	Model pincel("resources/objects/Pincel/pincel.obj");

	// --- Modelos Animados (Mixamo) ---
	ModelAnim hombre_sentado("resources/objects/Hombre_Sentado_Banca/hombre-sentado.dae");
	hombre_sentado.initShaders(animShader.ID); // Vincula el modelo al shader de animación
	ModelAnim mujer_sentada("resources/objects/Mujer_Sentada_Banca/mujer-sentada.dae");
	mujer_sentada.initShaders(animShader.ID);

	// --- Caballete (Cargado por partes para animación por keyframes) ---
	Model adorno("resources/objects/Caballete/adorno.obj");
	Model base("resources/objects/Caballete/base.obj");
	Model pataderecha("resources/objects/Caballete/pataderecha.obj");
	Model pataizquierda("resources/objects/Caballete/pataizquierda.obj");
	Model patatrasera("resources/objects/Caballete/patatrasera.obj");
	Model pintura("resources/objects/Caballete/pintura.obj");
	Model soportetrasero("resources/objects/Caballete/soportetrasero.obj");
	Model caballete_completo("resources/objects/Caballete/caballete_completo.obj"); // Modelo estático (de referencia)
	// --- Entorno y Vegetación ---
	Model mariposa("resources/objects/Mariposa/mariposa.obj");
	Model matteucia("resources/objects/Plantas/matteucia.obj");
	Model phormium("resources/objects/Plantas/phormium.obj");
	Model arbol_generico("resources/objects/Plantas/arbol_generico.obj");
	Model arbol_basico("resources/objects/Plantas/arbol_basico.obj");
	Model arbol_primaveral("resources/objects/Plantas/arbol_primaveral.obj");
	Model maceta("resources/objects/Plantas/maceta.obj");
	Model rosa("resources/objects/Plantas/rosa.obj");
	Model flor_narciso("resources/objects/Plantas/flor_narciso.obj");
	Model flor_anemonas("resources/objects/Plantas/flor_anemonas.obj");
	Model flor_nieve("resources/objects/Plantas/flor_nieve.obj");

	// =========================================================================
	// 7. INICIALIZACIÓN DE AUDIO (MINIAUDIO)
	// =========================================================================
	ma_engine engine;
	if (ma_engine_init(NULL, &engine) != MA_SUCCESS) {
		std::cerr << "Error al inicializar el motor de audio." << std::endl;
	}
	else {
		ma_sound sonido;
		// Carga el archivo de audio
		if (ma_sound_init_from_file(&engine, "resources/Audio/la_bruja_son_jarocho.mp3",
			0, NULL, NULL, &sonido) == MA_SUCCESS) {
			ma_sound_set_looping(&sonido, MA_TRUE);  // Configura para repetirse
			ma_sound_start(&sonido); // Inicia la reproducción
		}
		else {
			std::cerr << "Error al cargar el archivo de audio." << std::endl;
		}
	}

	// =========================================================================
	// 8. DEFINICIÓN DE SECUENCIAS DE KEYFRAMES
	// =========================================================================

	// -----------------------------------------------------------------
	// 8.1. ANIMACIÓN PRINCIPAL: CABALLETE (KeyFrame[0] a KeyFrame[20])
	// -----------------------------------------------------------------
	// Esta secuencia define el desensamble, rotación y reensamble del caballete.

	// --- KEYFRAME 0: Estado inicial (caballete armado) ---
	// Posición general en el mundo
	KeyFrame[0].posX = 3000.0f;
	KeyFrame[0].posY = 400.0f;
	KeyFrame[0].posZ = -2500.0f;
	// Posición (offset Y) de cada pieza (0 = en su sitio)
	KeyFrame[0].basePosY = 0.0f;
	KeyFrame[0].pataDerPosY = 0.0f;
	KeyFrame[0].pataIzqPosY = 0.0f;
	KeyFrame[0].soporteTrasPosY = 0.0f;
	KeyFrame[0].pataTrasPosY = 0.0f;
	KeyFrame[0].adornoPosY = 0.0f;
	KeyFrame[0].pinturaPosY = 0.0f;
	// Rotación de cada pieza (valores iniciales para que esté armado)
	KeyFrame[0].baseRot = -90.0f;
	KeyFrame[0].pataDerRot = 0.0f;
	KeyFrame[0].pataIzqRot = 0.0f;
	KeyFrame[0].soporteTrasRot = -10.0f;
	KeyFrame[0].pataTrasRot = 22.0f;
	KeyFrame[0].pinturaRot = -10.0f;
	KeyFrame[0].pinturaRotZ = 0.0f;
	KeyFrame[0].adornoRot = 90.0f;

	// --- KEYFRAMES 1 a 6: Desensamble progresivo ---
	// Cada pieza baja (posY = -500.0f) para "desaparecer"
	KeyFrame[1] = KeyFrame[0]; KeyFrame[1].basePosY = -500.0f;             
	KeyFrame[2] = KeyFrame[1]; KeyFrame[2].pataDerPosY = -500.0f;          
	KeyFrame[3] = KeyFrame[2]; KeyFrame[3].pataIzqPosY = -500.0f;          
	KeyFrame[4] = KeyFrame[3]; KeyFrame[4].soporteTrasPosY = -500.0f;      
	KeyFrame[5] = KeyFrame[4]; KeyFrame[5].pataTrasPosY = -500.0f;         
	KeyFrame[6] = KeyFrame[5]; KeyFrame[6].adornoPosY = -500.0f;           

	// --- KEYFRAMES 7 a 12: Movimiento y rotación de la pintura (sola) ---
	KeyFrame[7] = KeyFrame[6];
	KeyFrame[7].pinturaPosY = 350.0f;
	KeyFrame[7].pinturaRotZ = 45.0f;
	KeyFrame[7].pinturaRot = -15.0f;
	KeyFrame[7].posZ = -2380.0f;
	KeyFrame[7].posX = KeyFrame[0].posX - 20.0f;

	KeyFrame[8] = KeyFrame[7];
	KeyFrame[8].pinturaPosY = 500.0f;
	KeyFrame[8].pinturaRotZ = 180.0f;
	KeyFrame[8].pinturaRot = -20.0f;
	KeyFrame[8].posX = KeyFrame[7].posX + 10.0f;
	KeyFrame[8].posZ = -2340.0f;

	KeyFrame[9] = KeyFrame[8];
	KeyFrame[9].pinturaPosY = 600.0f;
	KeyFrame[9].pinturaRotZ = 360.0f;
	KeyFrame[9].pinturaRot = -10.0f;
	KeyFrame[9].posX = KeyFrame[8].posX - 10.0f;
	KeyFrame[9].posZ = -2360.0f;

	KeyFrame[10] = KeyFrame[9];
	KeyFrame[10].pinturaRotZ = 270.0f;
	KeyFrame[10].pinturaPosY = 500.0f;
	KeyFrame[10].posX = KeyFrame[0].posX - 10.0f;
	KeyFrame[10].posZ = -2400.0f;

	KeyFrame[11] = KeyFrame[10];
	KeyFrame[11].pinturaRotZ = 180.0f;
	KeyFrame[11].pinturaPosY = 350.0f;
	KeyFrame[11].posX = KeyFrame[0].posX - 20.0f;
	KeyFrame[11].posZ = -2480.0f;

	KeyFrame[12] = KeyFrame[11];
	KeyFrame[12].pinturaRotZ = 0.0f;
	KeyFrame[12].pinturaRot = -10.0f;
	KeyFrame[12].pinturaPosY = 0.0f;

	// --- KEYFRAMES 13 a 18: Reensamble del caballete ---
	// Las piezas regresan a su posición Y 0.0
	KeyFrame[13] = KeyFrame[12]; KeyFrame[13].adornoPosY = 0.0f;
	KeyFrame[14] = KeyFrame[13]; KeyFrame[14].pataTrasPosY = 0.0f;
	KeyFrame[15] = KeyFrame[14]; KeyFrame[15].soporteTrasPosY = 0.0f;
	KeyFrame[16] = KeyFrame[15]; KeyFrame[16].pataIzqPosY = 0.0f;
	KeyFrame[17] = KeyFrame[16]; KeyFrame[17].pataDerPosY = 0.0f;
	KeyFrame[18] = KeyFrame[17]; KeyFrame[18].basePosY = 0.0f;

	// --- KEYFRAMES 19 y 20: Finalización ---
	KeyFrame[19] = KeyFrame[0];      // Estado final (igual al inicial)
	KeyFrame[20] = KeyFrame[19];     // Duplicado (buffer de seguridad para la interpolación)

	// -----------------------------------------------------------------
	// 8.2. ANIMACIÓN SECUNDARIA: SILLA MECEDORA
	// -----------------------------------------------------------------
	SillaKeyFrame[0].rotSilla = 0.0f;   // Posición neutra
	SillaKeyFrame[1].rotSilla = 10.0f;  // Hacia adelante
	SillaKeyFrame[2].rotSilla = -10.0f; // Hacia atrás
	SillaKeyFrame[3].rotSilla = 0.0f;   // Regreso al centro
	i_max_steps_silla = 35; // Pasos entre cada keyframe

	// -----------------------------------------------------------------
	// 8.3. INICIALIZACIÓN DE MARIPOSAS
	// -----------------------------------------------------------------
	inicializarMariposas(15); // Crea 15 mariposas

	// ----------------------------------------------------------------
	// 8.4. ANIMACIÓN DEL PINCEL
	// -----------------------------------------------------------------
	PincelKeyFrame[0].posX = 2875.0f; PincelKeyFrame[0].posY = 380.0f; PincelKeyFrame[0].posZ = -1000.0f; PincelKeyFrame[0].rotZ = 0.0f;
	PincelKeyFrame[1].posX = 2885.0f; PincelKeyFrame[1].posY = 385.0f; PincelKeyFrame[1].posZ = -1020.0f; PincelKeyFrame[1].rotZ = 10.0f;
	PincelKeyFrame[2].posX = 2895.0f; PincelKeyFrame[2].posY = 390.0f; PincelKeyFrame[2].posZ = -1040.0f; PincelKeyFrame[2].rotZ = -10.0f;
	PincelKeyFrame[3].posX = 2885.0f; PincelKeyFrame[3].posY = 385.0f; PincelKeyFrame[3].posZ = -1010.0f; PincelKeyFrame[3].rotZ = 5.0f;
	PincelKeyFrame[4].posX = 2875.0f; PincelKeyFrame[4].posY = 380.0f; PincelKeyFrame[4].posZ = -1000.0f; PincelKeyFrame[4].rotZ = 0.0f;

	// Inicializa la posición del pincel en el primer frame
	posPincelX = PincelKeyFrame[0].posX;
	posPincelY = PincelKeyFrame[0].posY;
	posPincelZ = PincelKeyFrame[0].posZ;
	rotPincelZ = PincelKeyFrame[0].rotZ;

	// -----------------------------------------------------------------
	// 8.5. TEXTURAS DE PINTURA (para el lienzo)
	// -----------------------------------------------------------------
	texturaPintura[0] = t_rosa;
	texturaPintura[1] = t_rojo;
	texturaPintura[2] = t_verde;
	texturaPintura[3] = t_naranja;
	texturaPintura[4] = t_azul;
	pinturaActual = 0; // Inicia con la primera textura
	mezclaPintura = 0.5f; // nivel de mezcla entre capas

	// =========================================================================
	// 9. LAMBDA PARA DIBUJAR OBJETOS ESTÁTICOS
	// =========================================================================
	// Define una función "inline" (lambda) para agrupar todo el código de
	// dibujado de objetos estáticos y limpiar el bucle principal.

	auto drawStaticObjects = [&](Shader& staticShader) {
		glm::mat4 modelOp = glm::mat4(1.0f);

		// --- BANCA ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-2200.0f, 121.5f, -2150.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(60.0f, 39.1f, 60.0f));
		staticShader.setMat4("model", modelOp);
		banca.Draw(staticShader);

		// --- MUSEO ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-27.0f, 1.5f, 5.0f));
		modelOp = glm::scale(modelOp, glm::vec3(50.0f));
		staticShader.setMat4("model", modelOp);
		museo.Draw(staticShader);

		// --- VEGETACIÓN (Bucles para crear múltiples instancias) ---
		for (int i = 0; i < 9; i++) {
			float posX = 50.0f - i * 130.0f;
			float posY = 5.0f;
			float posZ = -1800.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(20.0f));
			staticShader.setMat4("model", modelOp);
			phormium.Draw(staticShader);
		}


		for (int i = 0; i < 9; i++) {
			float posX = 50.0f - i * 130.0f;
			float posY = 5.0f;
			float posZ = -1950.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(20.0f));
			staticShader.setMat4("model", modelOp);
			phormium.Draw(staticShader);
		}

		for (int i = 0; i < 7; i++) {
			float posX = -1700.0f + i * 210.0f;
			float posY = 5.0f;
			float posZ = -3200.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			modelOp = glm::scale(modelOp, glm::vec3(60.0f, 150.1f, 60.0f));

			staticShader.setMat4("model", modelOp);
			arbol_basico.Draw(staticShader);
		}

		struct LineaMatteucia {
			float posXInicial;
			float posZ;
			int cantidad;
		};

		std::vector<LineaMatteucia> lineas = {
			{2300.0f, 1620.0f, 22},
			{2300.0f, 1390.0f, 22},
			{2300.0f, 1160.0f, 22},
			{2300.0f,  930.0f, 22},
			{1800.0f,  700.0f, 20},
			{1800.0f,  470.0f, 20},
			{ 570.0f,  240.0f, 14},
			{ -190.0f,  10.0f, 11},
			{ -190.0f, -220.0f, 11},
			{ -190.0f, -450.0f, 11}
		};

		for (const auto& linea : lineas) {
			for (int i = 0; i < linea.cantidad; i++) {
				float posX = linea.posXInicial - i * 230.0f; 
				float posY = 5.0f;
				float posZ = linea.posZ;

				glm::mat4 modelOp = glm::mat4(1.0f);
				modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
				modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				modelOp = glm::scale(modelOp, glm::vec3(20.0f));

				staticShader.setMat4("model", modelOp);
				matteucia.Draw(staticShader);
			}
		}

		struct LineaArbolGenerico {
			float posXInicial;
			float posZ;
			int cantidad;
		};

		std::vector<LineaArbolGenerico> lineasArboles = {
			{420.0f, 1720.0f, 2},
			{710.0f, 1490.0f, 2},
			{260.0f, 1260.0f, 2},
			{530.0f,  1030.0f, 2},
			{140.0f,  600.0f, 2},
			{380.0f,  570.0f, 2},
			{ 100.0f,  340.0f, 2},
			{ -25.0f,  10.0f, 2},
			{ -10.0f, -20.0f, 2},
			{ -15.0f, -250.0f, 2}
		};

		for (const auto& linea : lineasArboles) {
			for (int i = 0; i < linea.cantidad; i++) {

				float separation = 1700.0f;
				float posX = linea.posXInicial - i * separation;
				float posY = 5.0f;
				float posZ = linea.posZ;

				glm::mat4 modelOp = glm::mat4(1.0f);
				modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
				modelOp = glm::scale(modelOp, glm::vec3(50.0f));

				staticShader.setMat4("model", modelOp);
				arbol_generico.Draw(staticShader);
			}
		}

		struct LineaRosas {
			float posXInicial;
			float posZ;
			int cantidad;
		};

		std::vector<LineaRosas> lineasRosas = {
			{620.0f, 1720.0f, 12},
			{910.0f, 1490.0f, 12},
			{460.0f, 1260.0f, 12},
			{730.0f,  1030.0f, 12},
			{340.0f,  600.0f, 12},
			{580.0f,  570.0f, 12},
			{ 100.0f,  340.0f, 12},
			{ -25.0f,  20.0f, 12},
			{ -10.0f, -10.0f, 12},
			{ -15.0f, -15.0f, 12}
		};

		for (const auto& linea : lineasRosas) {
			for (int i = 0; i < linea.cantidad; i++) {
				float separation = 230.0f;
				float posX = linea.posXInicial - i * separation;
				float posY = 5.0f;
				float posZ = linea.posZ;

				glm::mat4 modelOp = glm::mat4(1.0f);
				modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
				modelOp = glm::rotate(modelOp, glm::radians(80.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				modelOp = glm::scale(modelOp, glm::vec3(2.0f));

				staticShader.setMat4("model", modelOp);
				rosa.Draw(staticShader);
			}
		}

		struct LineaNieve {
			float posXInicial;
			float posZ;
			int cantidad;
		};

		std::vector<LineaNieve> lineasNieve = {
			{320.0f, 1420.0f, 12},
			{610.0f, 1190.0f, 12},
			{860.0f, 960.0f, 12},
			{430.0f,  730.0f, 12},
			{740.0f,  300.0f, 12},
			{280.0f,  270.0f, 12},
			{ 100.0f,  40.0f, 12},
			{ -25.0f,  10.0f, 12},
			{ -10.0f, -20.0f, 12},
			{ -15.0f, -120.0f, 12}
		};

		for (const auto& linea : lineasNieve) {
			for (int i = 0; i < linea.cantidad; i++) {
				float separation = 230.0f;
				float posX = linea.posXInicial - i * separation;
				float posY = 5.0f;
				float posZ = linea.posZ;

				glm::mat4 modelOp = glm::mat4(1.0f);
				modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
				modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				modelOp = glm::rotate(modelOp, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				modelOp = glm::scale(modelOp, glm::vec3(35.0f));

				staticShader.setMat4("model", modelOp);
				flor_nieve.Draw(staticShader);
			}
		}

		for (int i = 0; i < 3; i++) {
			float posX = 50.0f - i * 500.0f;
			float posY = 5.0f;
			float posZ = -1900.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(40.0f, 60.0f, 40.0f));

			staticShader.setMat4("model", modelOp);
			arbol_primaveral.Draw(staticShader);
		}


		for (int i = 0; i < 10; i++) {
			float posX = 1800.0f - i * 130.0f;
			float posY = 5.0f;
			float posZ = -2150.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(20.0f));
			staticShader.setMat4("model", modelOp);
			phormium.Draw(staticShader);
		}

		for (int i = 0; i < 10; i++) {
			float posX = 1800.0f - i * 130.0f;
			float posY = 5.0f;
			float posZ = -2310.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(20.0f));
			staticShader.setMat4("model", modelOp);
			phormium.Draw(staticShader);
		}

		for (int i = 0; i < 10; i++) {
			float posX = 1800.0f - i * 130.0f;
			float posY = 5.0f;
			float posZ = -2500.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, glm::vec3(posX, posY, posZ));
			modelOp = glm::scale(modelOp, glm::vec3(20.0f));
			staticShader.setMat4("model", modelOp);
			phormium.Draw(staticShader);
		}

		for (int i = 0; i < 6; i++) {
			float posX = 1850.0f - i * 260.0f;
			float posY = 5.0f;
			float posZ = -2480.0f;

			modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelOp = glm::scale(modelOp, glm::vec3(5.0f, 5.0f, 8.0f));
			staticShader.setMat4("model", modelOp);
			flor_anemonas.Draw(staticShader);
		}

		for (int i = 0; i < 6; i++) {
			float posX = 1850.0f - i * 260.0f;
			float posY = 5.0f;
			float posZ = -2150.0f;

			modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelOp = glm::scale(modelOp, glm::vec3(5.0f, 5.0f, 8.0f));
			staticShader.setMat4("model", modelOp);
			flor_anemonas.Draw(staticShader);
		}

		for (int i = 0; i < 6; i++) {
			float posX = 1850.0f - i * 260.0f;
			float posY = 5.0f;
			float posZ = -2290.0f;

			modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelOp = glm::scale(modelOp, glm::vec3(5.0f, 5.0f, 8.0f));
			staticShader.setMat4("model", modelOp);
			flor_anemonas.Draw(staticShader);
		}

		for (int i = 0; i < 5; i++) {
			float posX = 1750.0f - i * 280.0f;
			float posY = 5.0f;
			float posZ = -2380.0f;

			modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelOp = glm::scale(modelOp, glm::vec3(28.0f));
			staticShader.setMat4("model", modelOp);
			flor_narciso.Draw(staticShader);

		}

		for (int i = 0; i < 5; i++) {
			float posX = 1750.0f - i * 280.0f;
			float posY = 5.0f;
			float posZ = -2210.0f;

			modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, posZ));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			modelOp = glm::scale(modelOp, glm::vec3(28.0f));
			staticShader.setMat4("model", modelOp);
			flor_narciso.Draw(staticShader);

		}

		// --- MACETAS ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(880.0f, 140.0f, -3550.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2210.0f, 140.0f, -3550.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2520.0f, 140.0f, -2070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2530.0f, 140.0f, -800.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1370.0f, 140.0f, -1250.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(400.0f, 140.0f, -800.0f));
		modelOp = glm::scale(modelOp, glm::vec3(180.0f));
		staticShader.setMat4("model", modelOp);
		maceta.Draw(staticShader); 
		
		// --- PINTURAS (Posicionadas una por una) ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1095.0f, 580.0f, -3625.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_01.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1280.0f, 380.0f, -3070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_02.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2235.0f, 580.0f, -3625.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_03.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1280.0f, 580.0f, -3070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(70.0f));
		staticShader.setMat4("model", modelOp);
		pintura_04.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2970.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(80.0f));
		staticShader.setMat4("model", modelOp);
		pintura_05.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2770.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_06.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2580.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_07.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2240.0f, 420.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_08.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1460.0f, 370.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_09.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2200.0f, 390.0f, -3070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(113.0f));
		staticShader.setMat4("model", modelOp);
		pintura_10.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1460.0f, 540.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_11.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2200.0f, 580.0f, -3070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_12.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2240.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_13.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(610.0f, 580.0f, -3370.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_14.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1640.0f, 580.0f, -3625.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_15.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-630.0f, 580.0f, -930.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_16.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(190.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_17.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_18.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-210.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_19.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-410.0f, 580.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		pintura_20.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-630.0f, 580.0f, -1130.0f));
		modelOp = glm::scale(modelOp, glm::vec3(85.0f));
		staticShader.setMat4("model", modelOp);
		pintura_21.Draw(staticShader);

		// --- VITRINAS ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1750.0f, 365.0f, -3070.0f));
		modelOp = glm::rotate(modelOp, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(45.0f));
		staticShader.setMat4("model", modelOp);
		vitrina_01.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(1840.0f, 365.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(45.0f));
		staticShader.setMat4("model", modelOp);
		vitrina_02.Draw(staticShader);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(880.0f, 365.0f, -780.0f));
		modelOp = glm::rotate(modelOp, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(45.0f));
		staticShader.setMat4("model", modelOp);
		vitrina_03.Draw(staticShader);

		// --- CABALLETE ESTÁTICO (de referencia) ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2960.0f, 230.0f, -1000.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		caballete_completo.Draw(staticShader);
		};

	// =========================================================================
	// 10. MATRICES DE TRANSFORMACIÓN (Vista y Proyección)
	// =========================================================================
	glm::mat4 modelOp = glm::mat4(1.0f);		// Matriz de Modelo (reutilizable)
	glm::mat4 viewOp = glm::mat4(1.0f);			// Matriz de Vista (Cámara)
	glm::mat4 projectionOp = glm::mat4(1.0f);	// Matriz de Proyección (Perspectiva)

	// =========================================================================
	// 11. BUCLE DE RENDERIZADO (Game Loop)
	// =========================================================================
	while (!glfwWindowShouldClose(window))
	{
		skyboxShader.setInt("skybox", 0);

		// ------------------------------------
// 11.1. Control de Tiempo
// ------------------------------------
		lastFrame = SDL_GetTicks(); // Tiempo al inicio del frame (usando SDL)

		// ------------------------------------
		// 11.2. Actualizar Animaciones
		// ------------------------------------
		animate(); // Actualiza todas las variables de animación (posX, rotSilla, etc.)

		// ------------------------------------
		// 11.3. Limpieza de Pantalla
		// ------------------------------------
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Color de fondo (negro)
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Limpia buffers

		// ------------------------------------
		// 11.4. Configuración de Shaders y Luces
		// ------------------------------------

		// --- Shader Estático (staticShader) ---
		staticShader.use();

		// Luces (Pasa los datos de iluminación al shader)
		staticShader.setVec3("viewPos", camera.Position);
		staticShader.setVec3("dirLight.direction", lightDirection);
		staticShader.setVec3("dirLight.ambient", ambientColor);
		staticShader.setVec3("dirLight.diffuse", diffuseColor);
		staticShader.setVec3("dirLight.specular", glm::vec3(0.6f));

		// Luces puntuales (configuradas pero deshabilitadas/débiles)
		staticShader.setVec3("pointLight[0].position", lightPosition);
		staticShader.setVec3("pointLight[0].ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setVec3("pointLight[0].diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setVec3("pointLight[0].specular", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setFloat("pointLight[0].constant", 0.08f);
		staticShader.setFloat("pointLight[0].linear", 0.009f);
		staticShader.setFloat("pointLight[0].quadratic", 0.032f);

		staticShader.setVec3("pointLight[1].position", glm::vec3(-80.0, 0.0f, 0.0f));
		staticShader.setVec3("pointLight[1].ambient", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setVec3("pointLight[1].diffuse", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setVec3("pointLight[1].specular", glm::vec3(0.0f, 0.0f, 0.0f));
		staticShader.setFloat("pointLight[1].constant", 1.0f);
		staticShader.setFloat("pointLight[1].linear", 0.009f);
		staticShader.setFloat("pointLight[1].quadratic", 0.032f);

		// Luz Focal (Spotlight) - Animada por 'animate()'
		staticShader.setVec3("viewPos", camera.Position);
		staticShader.setVec3("spotLight[0].position", focoPos);
		staticShader.setVec3("spotLight[0].direction", focoDir);
		staticShader.setFloat("spotLight[0].cutOff", glm::cos(glm::radians(30.0f)));
		staticShader.setFloat("spotLight[0].outerCutOff", glm::cos(glm::radians(45.0f)));
		glm::vec3 lightBaseColor = glm::vec3(1.0f, 0.6f, 0.2f);
		// La intensidad (focoIntensidad) se multiplica para crear el pulso
		staticShader.setVec3("spotLight[0].ambient", lightBaseColor * 0.3f * focoIntensidad);
		staticShader.setVec3("spotLight[0].diffuse", lightBaseColor * 1.5f * focoIntensidad);
		staticShader.setVec3("spotLight[0].specular", lightBaseColor * 2.0f * focoIntensidad);
		staticShader.setFloat("spotLight[0].constant", 1.0f);
		staticShader.setFloat("spotLight[0].linear", 0.001f);
		staticShader.setFloat("spotLight[0].quadratic", 0.00005f);

		staticShader.setFloat("material_shininess", 32.0f);

		glm::mat4 tmp = glm::mat4(1.0f);
		// Matrices de Vista y Proyección(Perspectiva)
		projectionOp = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 10000.0f);
		viewOp = camera.GetViewMatrix();
		staticShader.setMat4("projection", projectionOp);
		staticShader.setMat4("view", viewOp);

		// --- Shader de Primitivas (myShader) ---
		myShader.use();
		viewOp = camera.GetViewMatrix();
		myShader.setMat4("view", viewOp);
		myShader.setMat4("projection", projectionOp);

		// --- Shader Animado (animShader) ---
		animShader.use();
		animShader.setMat4("projection", projectionOp);
		animShader.setMat4("view", viewOp);

		animShader.setVec3("material.specular", glm::vec3(0.5f));
		animShader.setFloat("material.shininess", 32.0f);
		animShader.setVec3("light.ambient", ambientColor);
		animShader.setVec3("light.diffuse", diffuseColor);
		animShader.setVec3("light.specular", 1.0f, 1.0f, 1.0f);
		animShader.setVec3("light.direction", lightDirection);
		animShader.setVec3("viewPos", camera.Position);

		// ------------------------------------
		// 11.5. Renderizado de la Escena
		// ------------------------------------

		// --- RENDERIZADO: Modelos Animados (Mixamo) ---
		// Hombre sentado
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-2100.0f, -2.0f, -2240.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(3.5f));
		animShader.setMat4("model", modelOp);
		hombre_sentado.Draw(animShader);

		// Mujer sentada
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(-2100.0f, -2.0f, -2040.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(3.5f));
		animShader.setMat4("model", modelOp);
		mujer_sentada.Draw(animShader);

		// --- RENDERIZADO: Primitivas (Piso) ---
		myShader.use();
		glBindVertexArray(VAO[2]); // Activa el VAO del piso
		modelOp = glm::scale(glm::mat4(1.0f), glm::vec3(40.0f, 2.0f, 40.0f)); // Escala masiva
		modelOp = glm::translate(modelOp, glm::vec3(0.0f, -1.0f, 0.0f)); // Baja un poco
		modelOp = glm::rotate(modelOp, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rota para que esté plano
		myShader.setMat4("model", modelOp);
		myShader.setVec3("aColor", 1.0f, 1.0f, 1.0f); // Color blanco (multiplica la textura)
		glBindTexture(GL_TEXTURE_2D, t_piedra); // Activa la textura del piso
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Dibuja el piso
		glBindVertexArray(0);

		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 10.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(5.0f, 5.0f, 1.0f));
		myShader.setMat4("model", modelOp);
		myShader.setVec3("aColor", 1.0f, 1.0f, 1.0f);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		
		// --- RENDERIZADO: Modelos Estáticos (Llamada a la Lambda) ---
		staticShader.use();
		staticShader.setMat4("projection", projectionOp);
		staticShader.setMat4("view", viewOp);

		drawStaticObjects(staticShader);

		// --- RENDERIZADO: Caballete Animado (por piezas) ---
		// Se usa 'playIndex' para obtener el estado actual desde KeyFrame[]
		glm::mat4 tmpPintura;   // Matriz padre (la pintura)
		float escala = 90.0f;   // Escala general

		// PINTURA (PADRE): Se aplica la posición, rotación Z animada y escala
		tmpPintura = glm::mat4(1.0f);
		tmpPintura = glm::translate(tmpPintura, glm::vec3(KeyFrame[playIndex].posX, KeyFrame[playIndex].posY, KeyFrame[playIndex].posZ));
		tmpPintura = glm::rotate(tmpPintura, glm::radians(KeyFrame[playIndex].pinturaRot), glm::vec3(0.0f, 0.0f, 1.0f));
		tmpPintura = glm::rotate(tmpPintura, glm::radians(pinturaRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
		tmpPintura = glm::scale(tmpPintura, glm::vec3(escala));
		staticShader.setMat4("model", tmpPintura);
		pintura.Draw(staticShader);

		// PIEZAS HIJAS: Se dibujan relativas a la matriz 'tmpPintura' (la pintura)
		// Se aplica el offset Y animado (ej. KeyFrame[playIndex].soporteTrasPosY)
		// ----- SOPORTE TRASERO -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.34f - 0.15f), (2.0f - 1.3f) + KeyFrame[playIndex].soporteTrasPosY, 0.0f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].soporteTrasRot), glm::vec3(0.0f, 0.0f, 1.0f));
		staticShader.setMat4("model", modelOp);
		soportetrasero.Draw(staticShader);

		// ----- ADORNO -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.52f - 0.15f), (3.0f - 1.3f) + KeyFrame[playIndex].adornoPosY, 0.0f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].adornoRot), glm::vec3(1.0f, 0.0f, 0.0f));
		staticShader.setMat4("model", modelOp);
		adorno.Draw(staticShader);

		// ----- BASE -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.0f - 0.15f), (0.66f - 1.3f) + KeyFrame[playIndex].basePosY, 0.0f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].baseRot), glm::vec3(1.0f, 0.0f, 0.0f));
		staticShader.setMat4("model", modelOp);
		base.Draw(staticShader);

		// ----- PATA DERECHA -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.0f - 0.15f), (-0.5f - 1.3f) + KeyFrame[playIndex].pataDerPosY, 0.4f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].pataDerRot), glm::vec3(0.0f, 0.0f, 1.0f));
		staticShader.setMat4("model", modelOp);
		pataderecha.Draw(staticShader);

		// ----- PATA IZQUIERDA -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.0f - 0.15f), (-0.5f - 1.3f) + KeyFrame[playIndex].pataIzqPosY, -0.4f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].pataIzqRot), glm::vec3(0.0f, 0.0f, 1.0f));
		staticShader.setMat4("model", modelOp);
		pataizquierda.Draw(staticShader);

		// ----- PATA TRASERA -----
		modelOp = tmpPintura * glm::translate(glm::mat4(1.0f), glm::vec3((0.81f - 0.15f), (0.0f - 1.3f) + KeyFrame[playIndex].pataTrasPosY, 0.0f));
		modelOp = glm::rotate(modelOp, glm::radians(KeyFrame[playIndex].pataTrasRot), glm::vec3(0.0f, 0.0f, 1.0f));
		staticShader.setMat4("model", modelOp);
		patatrasera.Draw(staticShader);

		// --- RENDERIZADO: Silla Mecedora (Animación independiente) ---
		glm::mat4 modelOp = glm::mat4(1.0f);
		modelOp = glm::translate(modelOp, glm::vec3(2910.0f, 320.0f, -3370.0f));
		modelOp = glm::rotate(modelOp, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		// Pivote de rotación:
		modelOp = glm::translate(modelOp, glm::vec3(0.0f, -100.0f, 0.0f));
		modelOp = glm::rotate(modelOp, glm::radians(rotSilla), glm::vec3(0.0f, 0.0f, 1.0f));
		modelOp = glm::translate(modelOp, glm::vec3(0.0f, 100.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(90.0f));
		staticShader.setMat4("model", modelOp);
		silla_mecedora.Draw(staticShader);


		// --- RENDERIZADO: Lámpara (Estática) y Foco ---
		modelOp = glm::translate(glm::mat4(1.0f), glm::vec3(2960.0f, 300.0f, -1500.0f));
		modelOp = glm::rotate(modelOp, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelOp = glm::scale(modelOp, glm::vec3(20.0f, 30.0f, 20.0f));
		staticShader.setMat4("model", modelOp);
		lampara.Draw(staticShader);

		// Actualiza la posición y dirección del foco (luz)
		glm::vec3 offsetFoco(0.0f, 150.0f, 0.0f);
		glm::vec3 focoPos = glm::vec3(2960.0f, 300.0f, -1500.0f) + offsetFoco;
		glm::vec3 focoDir = glm::normalize(glm::vec3(0.0f, -0.8f, -0.3f));

		// --- RENDERIZADO: Enjambre de Mariposas ---
		float tiempo = glfwGetTime(); 
		for (Mariposa& m : enjambre) {
			// Calcula el movimiento oscilante usando seno y coseno
			float vueloX = sin(tiempo * m.velocidad + m.fase) * 100.0f;
			float vueloY = sin(tiempo * 2.0f * m.velocidad + m.fase) * 30.0f;
			float vueloZ = cos(tiempo * m.velocidad + m.fase) * 100.0f;

			glm::vec3 posActual = m.posicionBase + glm::vec3(vueloX, vueloY, vueloZ);
			float rotY = sin(tiempo * m.velocidad + m.fase) * 45.0f;

			glm::mat4 modelOp = glm::mat4(1.0f);
			modelOp = glm::translate(modelOp, posActual);
			modelOp = glm::rotate(modelOp, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
			modelOp = glm::scale(modelOp, glm::vec3(m.escala));

			staticShader.setMat4("model", modelOp);
			mariposa.Draw(staticShader);
		}

		// --- RENDERIZADO: Pincel (Animado por 'animate()') ---
		glm::mat4 modelPincel = glm::mat4(1.0f);
		// Aplica la posición y rotación Z calculadas en 'animate()'
		modelPincel = glm::translate(modelPincel, glm::vec3(posPincelX, posPincelY, posPincelZ));
		modelPincel = glm::rotate(modelPincel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		modelPincel = glm::rotate(modelPincel, glm::radians(rotPincelZ), glm::vec3(0.0f, 0.0f, 1.0f));
		modelPincel = glm::scale(modelPincel, glm::vec3(50.0f));
		staticShader.use();
		staticShader.setMat4("model", modelPincel);
		pincel.Draw(staticShader);

		// --- RENDERIZADO: Lienzo (Primitiva VAO[0] con textura cambiante) ---
		myShader.use();
		glBindVertexArray(VAO[0]); // Cuadro plano que actúa como lienzo
		glm::mat4 modelLienzo = glm::mat4(1.0f);
		modelLienzo = glm::translate(modelLienzo, glm::vec3(2970.0f, 420.0f, -1000.0f)); // Posición
		modelLienzo = glm::rotate(modelLienzo, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotación Y
		modelLienzo = glm::rotate(modelLienzo, glm::radians(10.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Rotación X (inclinación)
		modelLienzo = glm::scale(modelLienzo, glm::vec3(200.0f, 180.0f, 1.0f)); // Escala
		myShader.setMat4("model", modelLienzo);
		myShader.setVec3("aColor", 1.0f, 1.0f, 1.0f);

		// ¡Importante! Activa la textura que 'pinturaActual' indique
		glBindTexture(GL_TEXTURE_2D, texturaPintura[pinturaActual]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);


		// ------------------------------------
		// 11.6. Renderizado del Skybox (SIEMPRE AL FINAL)
		// ------------------------------------
		skyboxShader.use();
		skybox.Draw(skyboxShader, viewOp, projectionOp, camera);

		// ------------------------------------
		// 11.7. Control de FPS y Buffers
		// ------------------------------------
		deltaTime = SDL_GetTicks() - lastFrame; // Tiempo que tardó el frame
		if (deltaTime < LOOP_TIME)
		{
			SDL_Delay((int)(LOOP_TIME - deltaTime));
		}

		// Intercambia los buffers (frontal y trasero) y sondea eventos
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	
	// =========================================================================
	// 12. LIMPIEZA
	// =========================================================================
	glDeleteVertexArrays(2, VAO);
	glDeleteBuffers(2, VBO);
	ma_engine_init(NULL, &engine);

}

//-------------------------------------------------------------------------------------
// 14. FUNCIONES DE CALLBACK (Implementación)
//-------------------------------------------------------------------------------------
/**
 * @brief Callback para el teclado. Se activa al presionar teclas.
 */
void my_input(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, (float)deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, (float)deltaTime);

	// --- VISTAS RÁPIDAS (MARCADORES DE CÁMARA) ---
	// Mueven la cámara a posiciones predefinidas (solo al presionar)
	if (key == GLFW_KEY_F && action == GLFW_PRESS) // Vista de la fachada
	{
		camera.Position = glm::vec3(0.0f, 500.0f, -6000.0f);
		camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_I && action == GLFW_PRESS) // Vista desde la entrada
	{
		camera.Position = glm::vec3(500.0f, 500.0f, -3200.0f);
		camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_1 && action == GLFW_PRESS) // Vista a la animación de la silla mecedora
	{
		camera.Position = glm::vec3(2430.0f, 310.0f, -3370.0f);
		camera.Front = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_2 && action == GLFW_PRESS) // Vista a la animación del caballete que se arma
	{
		camera.Position = glm::vec3(2490.0f, 340.0f, -2500.0f); 
		camera.Front = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_3 && action == GLFW_PRESS) // Vista a la animación de la lámpara
	{
		camera.Position = glm::vec3(2490.0f, 340.0f, -1500.0f);
		camera.Front = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_4 && action == GLFW_PRESS) // Vista a la animación del caballete con pincel
	{
		camera.Position = glm::vec3(2560.0f, 350.0f, -1000.0f);
		camera.Front = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);

	}

	if (key == GLFW_KEY_5 && action == GLFW_PRESS) // Vista a la animación de las personas
	{
		camera.Position = glm::vec3(1000.0f, 1100.0f, -2340.0f);
		camera.Front = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS) // Vista a la banca con personajes animados
	{
		camera.Position = glm::vec3(-500.0f, 300.0f, -2150.0f);
		camera.Front = glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_J && action == GLFW_PRESS) // Vista general al jardín
	{
		camera.Position = glm::vec3(-900.0f, 3000.0f, -70.0f);
		camera.Front = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 0.0f, -1.0f);
	}

	if (key == GLFW_KEY_V && action == GLFW_PRESS) // Vista a una vitrina
	{
		camera.Position = glm::vec3(880.0f, 365.0f, -1350.0f);
		camera.Front = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	if (key == GLFW_KEY_P && action == GLFW_PRESS) // Vista a la pintura principal de Frida y Diego
	{
		camera.Position = glm::vec3(980.0f, 580.0f, -3370.0f);
		camera.Front = glm::normalize(glm::vec3(-1.0f, 0.0f, 0.0f));
		camera.Up = glm::vec3(0.0f, 1.0f, 0.0f);
	}

	// --- CONTROLES DE ANIMACIÓN ---
	// 'Z': Inicia/Detiene la animación principal del caballete
	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		if (play == false && (FrameIndex > 1))
		{
			std::cout << "Play animation" << std::endl;
			resetElements(); // Vuelve al estado inicial (KeyFrame[0])
			interpolation(); // Calcula los incrementos para el primer tramo
			play = true;     // Activa la bandera
			playIndex = 0;
			i_curr_steps = 0;
		}
		else
		{
			play = false;
			std::cout << "Not enough Key Frames" << std::endl;
		}
	}
	// 'E': Inicia/Detiene la animación de la Silla Mecedora
	if (key == GLFW_KEY_E && action == GLFW_PRESS) { 
		playSilla = !playSilla;
		if (playSilla) {
			playIndexSilla = 0;
			sillaInterpT = 0.0f;
			firstPlaySilla = true;
		}
	}

	// 'Q': Inicia/Detiene la animación del Pincel
	if (key == GLFW_KEY_Q && action == GLFW_PRESS) {
		playPincel = !playPincel;
		if (playPincel) {
			playIndexPincel = 0;
			pincelInterpT = 0.0f;
			firstPlayPincel = true;
		}
	}


}

/**
 * @brief Callback para el reajuste de la ventana.
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	// Ajusta el viewport al nuevo tamaño
	glViewport(0, 0, width, height);
}

/**
 * @brief Callback para el movimiento del ratón.
 */
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	// Calcula el desplazamiento (offset) desde el último frame
	double xoffset = xpos - lastX;
	double yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	// Pasa el offset a la cámara para que calcule la nueva orientación
	camera.ProcessMouseMovement(xoffset, yoffset);
}

/**
 * @brief Callback para la rueda del ratón (scroll).
 */
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	// Pasa el offset Y (scroll vertical) a la cámara para hacer zoom
	camera.ProcessMouseScroll(yoffset);
}
