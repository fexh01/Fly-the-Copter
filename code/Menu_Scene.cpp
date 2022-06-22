/*
 * MENU SCENE
 * Copyright © 2022+ Félix Hernández Muñoz-Yusta
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * felixhernandezmy@gmail.com
 */

#include "Menu_Scene.hpp"
#include "Game_Scene.hpp"
#include <basics/Canvas>
#include <basics/Director>
#include <basics/Transformation>

using namespace basics;
using namespace std;

namespace flythecopter
{

    // Constructor de la escena
    Menu_Scene::Menu_Scene()
    {
        state         = LOADING;
        suspended     = true;
        canvas_width  = 1280;
        canvas_height =  720;
        ayuda = false;
    }

    // Aquí se inicializan los atributos que deben restablecerse cada vez que se inicia la escena.

    bool Menu_Scene::initialize ()
    {
        for (auto & option : options)
        {
            option.is_pressed = false;
        }
        return true;
    }




    // Este método se invoca automáticamente una vez por fotograma cuando se acumulan eventos dirigidos a la escena.
    void Menu_Scene::handle (basics::Event & event)
    {
        if (state == READY)                     // Se descartan los eventos cuando la escena está LOADING
        {
            switch (event.id)
            {
                case ID(touch-started):         // El usuario toca la pantalla
                case ID(touch-moved):
                {
                    // Se determina qué opción se ha tocado:

                    Point2f touch_location = { *event[ID(x)].as< var::Float > (), *event[ID(y)].as< var::Float > () };
                    int     option_touched = option_at (touch_location);

                    // Solo se puede tocar una opción a la vez (para evitar selecciones múltiples),
                    // por lo que solo una se considera presionada (el resto se "sueltan"):

                    for (int index = 0; index < number_of_options; ++index)
                    {
                        options[index].is_pressed = index == option_touched;
                    }

                    break;
                }

                case ID(touch-ended):           // El usuario deja de tocar la pantalla
                {
                    // toggle entre el menu y las instrucciones
                    if(ayuda){
                        ayuda = false;
                    }
                    // Se "sueltan" todas las opciones:

                    for (auto & option : options) option.is_pressed = false;

                    // Se determina qué opción se ha dejado de tocar la última y se actúa como corresponda:

                    Point2f touch_location = { *event[ID(x)].as< var::Float > (), *event[ID(y)].as< var::Float > () };

                    if (option_at (touch_location) == PLAY && !ayuda)
                    {
                        director.run_scene (shared_ptr< Scene >(new Game_Scene));
                    }
                    else if (option_at (touch_location) == AYUDA){
                        ayuda = true;
                    }

                    break;
                }
            }
        }
    }



    // Este método se invoca automáticamente una vez por fotograma para que la escena actualize su estado.
    void Menu_Scene::update (float time)
    {
        if (!suspended) if (state == LOADING)
        {
            Graphics_Context::Accessor context = director.lock_graphics_context ();

            if (context)
            {
                // Asigna una imagen a las texturas
                PlayButton_texture = Texture_2D::create (0, context, "PlayButton.png");
                CopterLogo_texture = Texture_2D::create (0, context, "CopterLogo.png");
                Ayuda_texture = Texture_2D::create (0, context, "ayuda.png");
                Texto_texture = Texture_2D::create (0, context, "texto.png");

                // En caso de que las texturas esten cargadas, la escena entra en estado READY
                if (PlayButton_texture && CopterLogo_texture && Ayuda_texture && Texto_texture)
                {
                    context->add (PlayButton_texture);
                    context->add (CopterLogo_texture);
                    context->add (Ayuda_texture);
                    context->add (Texto_texture);
                    state = READY;

                }else{
                    state = ERROR;
                }

                if (state == READY)
                {
                    configure_options();
                }
            }
        }
    }


    // Este método se invoca automáticamente una vez por fotograma para que la escena dibuje su contenido.
    void Menu_Scene::render (Graphics_Context::Accessor & context)
    {
        if (!suspended)
        {
            // El canvas se puede haber creado previamente, en cuyo caso solo hay que pedirlo:
            Canvas * canvas = context->get_renderer< Canvas > (ID(canvas));

            // Si no se ha creado previamente, hay que crearlo una vez:
            if (!canvas)
            {
                 canvas = Canvas::create (ID(canvas), context, {{ canvas_width, canvas_height }});
            }

            // Si el canvas se ha podido obtener o crear, se puede dibujar con él:
            if (canvas)
            {
                canvas->clear ();

                if (state == READY)
                {
                    if(!ayuda){
                        // Dibuja el menu
                        if (PlayButton_texture && CopterLogo_texture){
                            canvas->fill_rectangle
                                    (
                                            {canvas_width * .5f, canvas_height * .7f},
                                            {(CopterLogo_texture->get_width ())*1.f, (CopterLogo_texture->get_height ())*1.f },
                                            CopterLogo_texture. get ()
                                    );
                            canvas->fill_rectangle
                                    (
                                            {options[0].position[0], options[0].position[1]},
                                            {(PlayButton_texture->get_width ()), (PlayButton_texture->get_height ()) },
                                            PlayButton_texture. get ()
                                    );
                            canvas->fill_rectangle
                                    (
                                            {options[1].position[0], options[1].position[1]},
                                            {(Ayuda_texture->get_width ()), (Ayuda_texture->get_height ()) },
                                            Ayuda_texture. get ()
                                    );
                        }
                    }
                    // Dibuja las instrucciones
                    else{
                        canvas->fill_rectangle
                                (
                                        { canvas_width * .5f, canvas_height * .5f },
                                        { (Texto_texture->get_width ()), (Texto_texture->get_height ()) },
                                        Texto_texture. get ()
                                );
                    }

                }
            }
        }
    }

    // Establece las propiedades de cada opción
    void Menu_Scene::configure_options ()
    {
        // Se calcula la altura total del menú:
        float menu_height = 0;
        for (auto & option : options) menu_height += (PlayButton_texture->get_height ())*2;


        // Se calcula la posición del borde superior del menú en su conjunto de modo que
        // quede centrado verticalmente:
        float option_top = canvas_height / 20.f + menu_height / 2.5f;


        // Se establece la posición del borde superior de cada opción:
        for (unsigned index = 0; index < number_of_options; ++index)
        {
            options[index].position = Point2f{ canvas_width / 2.f, option_top };

            option_top -= (PlayButton_texture->get_height ());
        }


        // Se restablece la presión de cada opción:
        initialize ();
    }


    // Devuelve el índice de la opción que se encuentra bajo el punto indicado.
    int Menu_Scene::option_at (const Point2f & point)
    {
        for (int index = 0; index < number_of_options; ++index)
        {
            const Option & option = options[index];

            if
            (
                point[0] > option.position[0] - PlayButton_texture->get_width()  &&
                point[0] < option.position[0] + PlayButton_texture->get_width()  &&
                point[1] > option.position[1] - PlayButton_texture->get_height() &&
                point[1] < option.position[1] + PlayButton_texture->get_height()
            )
            {
                return index;
            }
        }
        return -1;
    }
}
