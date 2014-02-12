face-tracking-mouse-control-opencv
==================================

Mouse control with face tracking and OpenCV

Descripción 
===========
El programa realiza seguimiento de la cara del usuario a través de la webcam con el objetivo
de manipular el ratón con los movimientos de éste.

Uso
===
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

Autores
=======
Yuri Garcés Ciemerozum
Teresa Hirzle
Francisco Vélez Ocaña

Nota
====
Para la construcción de las funciones detect() y track() se ha utilizado parte
del código creado por "bsdnoobz", disponible en: 
https://github.com/bsdnoobz/opencv-code/blob/master/eye-tracking.cpp
