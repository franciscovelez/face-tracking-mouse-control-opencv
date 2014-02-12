/*
   Descripción: 
   El programa realiza seguimiento de la cara del usuario a través de la webcam con el objetivo
   de manipular el ratón con los movimientos de éste.

   Uso:
   En el documento PDF se explica con detalle cómo usar el programa. A grandes rasgos, el puntero
   se controla con la dirección a la que apunte la nariz y los botones se controlan con los 
   siguientes gestos:
       - Clic con el botón izquierdo: Levantar y bajar las cejas rápidamente (aproximadamente un 
         poco menos de un segundo). 
       - Doble clic con el botón izquierdo: Mantener las cejas levantadas hasta que se realice la 
         acción (Por defecto, aproximadamente algo más de un segundo).
       - Arrastrar y soltar: Cerrar los ojos hasta escuchar un pitido, mover el puntero lo que se 
         desee y para soltar, volver a cerrar los ojos hasta escuchar un pitido.
       - Clic con el botón derecho: Cerrar los ojos hasta escuchar un pitido (como con arrastrar y 
         soltar) y levantar las cejas hasta que se realice la acción (aproximadamente algo más de 
         un segundo).

  Autores:
  Yuri Garcés Ciemerozum
  Teresa Hirzle
  Francisco Vélez Ocaña
  
  Nota: 
  Para la construcción de las funciones detect() y track() se ha utilizado parte
	del código creado por "bsdnoobz", disponible en: 
  https://github.com/bsdnoobz/opencv-code/blob/master/eye-tracking.cpp
*/

#include <opencv2/opencv.hpp>
#include <opencv2/nonfree/features2d.hpp>
#include <windows.h>
#include <fstream>
#include <time.h>

using namespace std;
using namespace cv;

//Ruta a los ficheros de datos para entrenar el clasificador
string nose_name     =  "haarcascades/haarcascade_mcs_nose.xml";
string eye_pair_name =  "haarcascades/haarcascade_mcs_eyepair_small.xml";

//Variables globales para controlar el comportamiento de los parámetros del menú de opciones
int    mouseSpeed =  5, //Velocidad del puntero
       sensClick  = 50, //Sensibilidad en la detección del clic
       sensDrag   = 50, //Sensibilidad en la detección del arrastrar y soltar
       filtroBlur =  0, //Sensibilidad al ruido de la imagen
       calibrado  =  1, //Control de reinicialización de la calibración
       debug      =  0, //Modo debug para mostrar una ventana con lo que captura la webcam
       umbralMov  =  5; //Control sobre cuando se puede mover el ratón

string titleWin("Configuración del ratón");  //Título de la ventana del menú de opciones

/* 
   Toma una imagen y calcula los puntos clave usando el algoritmo SIFT
   Entrada: imgIn.     Imagen a la que extraer los puntos
   Salida:  keypoints (return). Vector de keyPoint con los puntos clave
*/
vector<KeyPoint> aplicarSift(Mat & imgIn){
  Mat img;                     //Imagen temporal
  vector<KeyPoint> keypoints;  //Vector para almacenar los keyPoint
  //Parámetros para SIFT
  int    nFeatures     = 0;    //Nº características a retener
  int    nOctaveLayers = 3;    //Capas por nivel
  double contrThres    = 0.01; //Umbral contraste
  double edgeThres     = 10.0; //Umbral de bordes
  double sigma         = 1.6;  //Sigma para gaussiana
  
  //Pasamos a gris la imagen
  cvtColor(imgIn, img, CV_RGB2GRAY); 

  //Definimos objeto SIFT y extraemos los puntos
  SIFT sift(nFeatures, nOctaveLayers, contrThres, edgeThres, sigma); 
  sift(img, noArray(), keypoints, noArray(), false);  

  return keypoints;
}


/* 
   Dado un clasificador, detecta el patrón en la imagen y devuelve un rectángulo
   del área detectada
   Entrada: im.   Imagen en la que buscar el patrón
      rect.       Rectángulo que indica el área en la imagen en la que se ha 
                  detectado el patrón
      classifier. Clasificador a usar para detectar el patrón     
   Salida:  rect (sobrescrito). Rectángulo con el área detectada
*/
void detect(Mat& im, Rect& rect, CascadeClassifier &classifier){  
  vector<Rect> detected;

  //Lanzamos el detector
  classifier.detectMultiScale(im, detected, 1.1, 4, CV_HAAR_DO_CANNY_PRUNING);
  //Si encontramos algo, guardamos el rectángulo del área
  if(detected.size()>0) rect = detected[0];
  else                  rect = Rect(0,0,0,0);
}

/* 
   Realiza tracking de una imagen, dado el objeto a seguir (part_template) y el rectángulo en el que 
   estaba en la imagen anterior
   Entrada: im.            Imagen en la que buscar el patrón
            part_template. objeto a buscar
            rect.          Rectángulo que indica el área en la imagen en la que se ha 
                           detectado el patrón
   Salida:  rect (sobrescrito). Rectángulo con el área detectada
*/
void track(Mat& im, Mat& part_template, Rect& rect){  
  Size   size(rect.width * 2, rect.height * 2);
  Rect   window(rect + size - Point(size.width/2, size.height/2));
  Mat    dst(rect.width, rect.height, CV_32FC1);
  double minval, maxval;
  Point  minloc, maxloc;    

  //Definimos la ventana
  window &= Rect(0, 0, im.cols, im.rows);
  //Buscamos el patrón
  matchTemplate(im(window), part_template, dst, CV_TM_SQDIFF_NORMED);  
  //De los patrones detectados, buscamos el que mejor corresponde
  minMaxLoc(dst, &minval, &maxval, &minloc, &maxloc);
  //Si la región detectada no supera el umbral, se acepta
  if (minval <= 0.2){
    rect.x = window.x + minloc.x;
    rect.y = window.y + minloc.y;
  } else 
    rect.x = rect.y = rect.width = rect.height = 0;
}

/*
   Calcula y muestra en pantalla los frames por segundo a los que está
   capturando la cámara
*/
void printFPS(){
  static int    cont = 0;       //Contador global
  static time_t t0   = time(0), //Temporizadores globales
              t1;

  //Contabilizamos la hora actual y el frame
  t1 = time(0);
  cont++;
  //Si ha transcurrido un segundo, mostramos en pantalla el número de frames
  if(t1-t0!=0){
    t0=t1;
    system("cls");
    cout << "Capturando ..." << endl << "FPS: " << cont << endl;
    cont = 0;
  }
}

/*
   Añade la ventana con el menú de opciones
*/
void actualizarSlider(){   
  createTrackbar("Velocidad",  titleWin, &mouseSpeed,  10);
  createTrackbar("Umbral Mov", titleWin, &umbralMov,   10);
  createTrackbar("Sens.Click", titleWin, &sensClick,  100);
  createTrackbar("Sens.Drag",  titleWin, &sensDrag,   100);
  createTrackbar("MedianBlur", titleWin, &filtroBlur,   8);
  createTrackbar("Calibrado",  titleWin, &calibrado,    1);
  createTrackbar("Debug",      titleWin, &debug,        1);
}
void ponerSlider(){  
  //Creamos la ventana, la redimensionamos y situamos en el punto que deseamos
  namedWindow( titleWin, CV_WINDOW_NORMAL);
  resizeWindow(titleWin, 400, 350);
  moveWindow(  titleWin, 652, 205);
  actualizarSlider();
  
}


int main ( int argc, char* argv[]){
  vector<KeyPoint>  keypointsFrame; 
  vector<time_t>    tiempos;
  VideoCapture      capture(0);     //Inicializamos la webcam 0
  CascadeClassifier nose,           //Clasificador para la nariz
                    eye_pair;       //Clasificador para el par de ojos
  Rect  rect_nose,
        rect_eye_pair;
  Point punto_cero,
        anterior,
        dif;      
  Mat   nose_template, 
        face_template, 
        frame, 
        cara, 
        ojos;

  double fact,
         dx, 
         dy;
  int    TAM_TIEMPOS = 5,
         frameCont   = 0,
         posT        = 0,
         MAX         = 50,
         min         = 1000,
         max         = 0,
         nSift,
         dift,
         minMarg, 
         maxMarg;  
  bool   movimiento = false,  
         drag       = false;


  if(!capture.isOpened()) 
    cout << "Camara no detectada" << endl;
  else{
    //Configuramos la cámara a 5 FPS
    capture.set(CV_CAP_PROP_FPS  ,5);
    //Inicializamos los clasificadores
    nose     =  CascadeClassifier(nose_name);
    eye_pair =  CascadeClassifier(eye_pair_name);
    //Cargamos el menu de opciones
    ponerSlider();

    //cvNamedWindow("WebCam", 1 ); //Descomentar para visualizar webcam
    //moveWindow("WebCam", 0, 210); //Descomentar para visualizar webcam

    cout << "Antes de comenzar se recomienda que adopte una postura lo mas natural " << endl
         << "posible procurando que la WebCam capture la cara frontalmente."         << endl
         << "Previamente al reconocimiento de gestos es necesario tomar algunos "    << endl
         << "valores de calibracion. Para ello basta con que permanezca en la "      << endl
         << "posicion inicial hasta que suene un pitido. "                           << endl
         << "No se preocupe si se ha movido durante la calibracion pues esta "       << endl
         << "continuara en cuanto vuelva a la posicion central"                      << endl
         << "Capturando ..."                                                         << endl;

    //Inicializamos el contador de tiempos
    for(int i=0; i<TAM_TIEMPOS;i++)  
      tiempos.push_back(time(0));    
        
    //Comenzamos el bucle de reconocimiento hasta que se  cierre la ventana
    while(IsWindowVisible((HWND)cvGetWindowHandle(titleWin.c_str()))){
      //Capturamos un frame
      capture >> frame;
      medianBlur(frame,frame,1+filtroBlur*2);
      flip((Mat)frame, (Mat)frame,1);
      if(calibrado==1){
        calibrado = 0;
        frameCont = 0;
        min       = 1000;
        max       = 0;
        rect_eye_pair.width = 0;
      }
      //Si no tenemos detectado el par de ojos ya sea por reconocimiento o por seguimiento
      //lo detectamos y ampliamos el recuadro detectado para incluir ojos y nariz
      if(rect_eye_pair.width==0){
        detect(frame, rect_eye_pair, eye_pair);
        rect_eye_pair.y -= rect_eye_pair.height;

        if(rect_eye_pair.y<0) 
          rect_eye_pair.y=0;

        rect_eye_pair.height *= 4;
        if(rect_eye_pair.y+rect_eye_pair.height >= frame.rows)
          rect_eye_pair.height = frame.rows-rect_eye_pair.y-1;

        //nos quedamos con el recuadro detectado ampliado de la imagen para el seguimiento
        if(rect_eye_pair.width!=0) 
          face_template = frame(rect_eye_pair).clone();      
      }
      //Entramos aquí si en en el anterior fotograma no se ha perdido el seguimiento o bien
      //se ha perdido pero se ha vuelto a detectar el par de ojos 
      if(rect_eye_pair.width!=0) 
        track(frame, face_template, rect_eye_pair);  

      //Entramos si tenemos localizado el par de ojos (por detección o por seguimiento)
      //Dado que se ha extendido el área detectado para el par de ojos en lo adelante
      //se referirá a él como a la cara
      if(rect_eye_pair.width!=0){
        cara = frame(rect_eye_pair);
        //Buscamos la nariz en la cara previamente detectada
        if(rect_nose.width==0){
          detect(cara, rect_nose, nose);
          //Una vez detectada la nariz, fijamos el punto de referencia para calcular el movimiento producido
          if(rect_nose.width!=0){
            nose_template = cara(rect_nose).clone();
            punto_cero    = Point(rect_nose.x+rect_eye_pair.x, rect_nose.y+rect_eye_pair.y);
            anterior.x    = rect_nose.x+rect_eye_pair.x;
            anterior.y    = rect_nose.y+rect_eye_pair.y;
          }
        }
        //Entramos aquí si en en el anterior fotograma no se ha perdido el seguimiento o bien
        //se ha perdido pero se ha vuelto a detectar la nariz
        if(rect_nose.width!=0){
          track(cara, nose_template, rect_nose);
          if(debug) rectangle(cara,rect_nose,Scalar(255,0,0));
          //Si se ha producido un movimiento muy brusco forzamos que se vuelva a detectar la cara
          if(abs(rect_nose.x+rect_eye_pair.x-anterior.x)>MAX || abs(rect_nose.y+rect_eye_pair.y-anterior.y)>MAX){
            rect_eye_pair.width = 0;
          }else{
            anterior.x = rect_nose.x+rect_eye_pair.x;
            anterior.y = rect_nose.y+rect_eye_pair.y;
          }
          //Calculamos la diferencia entre el punto de referencia y el actual en el seguimiento
          dif.x = rect_nose.x+rect_eye_pair.x-punto_cero.x;
          dif.y = rect_nose.y+rect_eye_pair.y-punto_cero.y;
          dx    = dy = 0;
          //Dejamos un umbral para el comienzo del movimiento del ratón para permitir
          //el reconocimiento de gestos.
          if(abs(dif.x)>5+umbralMov){
            if(dif.x> MAX) dif.x =  MAX;
            if(dif.x<-MAX) dif.x = -MAX;

            dx    = ((mouseSpeed+1)/100.0)*dif.x;
            fact  = pow(1.05,abs(dif.x)-10);
            dx   *= fact;
          }
          if(abs(dif.y)>5+umbralMov){
            if(dif.y> MAX) dif.y =  MAX;
            if(dif.y<-MAX) dif.y = -MAX;

            dy    = ((mouseSpeed+1)/100.0)*dif.y;
            fact  = pow(1.05,abs(dif.y)-10);
            dy   *= fact;
          }
          //Actualizamos la variable movimiento para bloquear o no el reconocimiento de gestos
          //En caso de que se haya producido movimiento realizamos la llamada al sistema para
          //mover el ratón
          if(dx==0 && dy==0) 
            movimiento = false;
          else{
            mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
            movimiento = true;
          }
        }
        //Aquí detectamos los gestos de clic (derecho e izquierdo) y drag&drop en caso de que
        //no estemos en movimiento
        if(!movimiento && rect_nose.width!=0){
          //Para ahorrar cálculo buscamos puntos SIFT sólo en el área de los ojos
          ojos           = cara(Rect(0, 0, cara.cols, cara.rows/2));
          keypointsFrame = aplicarSift(ojos);
          nSift          = keypointsFrame.size();
          if(debug){
            for(int i=0; i<nSift; i++)
              circle(ojos,keypointsFrame[i].pt,3,Scalar(0,255,0));
          }
          //Aquí se entra durante el proceso de calibración. Se registran el valor máximo y mínimo
          //de puntos SIFT detectados en los ojos mientras el usuario no está realizando ningún gesto
          if(frameCont<40){
            frameCont++;
            if(nSift>max) max = nSift;
            if(nSift<min) min = nSift;
            if(frameCont==40){ 
              frameCont=100; 
              cout << "Calibracion completada\a" << endl;
              actualizarSlider();
            }
          }
          //Reconocimiento de gestos una vez completada la calibración
          else{
            //Variamos la calibración en función de los valores de sensibilidad configurados
            //en los slider
            minMarg =  min*(sensDrag -50)/50;
            maxMarg = -max*(sensClick-50)/50;
            //Con la variable frameCont evitamos el reconocimiento repetitivo indeseado de los gestos
            if(frameCont<100) 
              frameCont++;
            //Reconocimiento del gesto de cejas subidas. Se cumple si estamos detectando más puntos SIFT
            //que el máximo detectado durante la calibración
            if((nSift>max+maxMarg && frameCont>=100)){
              //clic izquierdo
              if(!drag){
                frameCont = 98;
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0,0,0,0);
                Sleep(10);
                mouse_event(MOUSEEVENTF_LEFTUP,   0,0,0,0);
              }
              //clic derecho
              else{
                drag      = false;
                frameCont = 90;
                mouse_event(MOUSEEVENTF_RIGHTDOWN, 0,0,0,0);
                Sleep(10);
                mouse_event(MOUSEEVENTF_RIGHTUP,   0,0,0,0);
              }
            }
            //Reconocimiento del gesto de ojos cerrados. Se cumple si estamos detectando menos puntos SIFT
            //que el mínimo detectado durante la calibración. Aquí se controla mediante el vector tiempos
            //que durante el último segundo los ojos hayan estado cerrados para evitar falsos positivos
            //en caso de que el usuario parpadee
            if(nSift<min+minMarg){
              tiempos[posT] = time(0);            
              dift          = tiempos[posT]-tiempos[(posT+1)%TAM_TIEMPOS];
              posT          = (posT+1)%TAM_TIEMPOS;
              //Entramos si el valor más reciente y el más antiguo del vector tiempos han sido registrados
              //con una diferencia menor o igual a un segundo
              if(dift<=1 && frameCont>=100){                
                if(drag) mouse_event(MOUSEEVENTF_LEFTUP,   0,0,0,0);
                else     mouse_event(MOUSEEVENTF_LEFTDOWN, 0,0,0,0);

                drag      = !drag;
                frameCont = 90;
                cout << "\a"; //Emitimos un pitido
              }
            }          
          }
        }
      }
      if(debug){
        imshow( "WebCam", frame);
        printFPS();           
      }
      waitKey(10);
    }

    capture.release();
    if(debug)
      destroyWindow("WebCam");      //Descomentar para visualizar webcam
    destroyWindow(titleWin);
  }
  return 0;
}