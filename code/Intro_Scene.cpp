/*
 * INTRO SCENE
 * Copyright © 2018+ Ángel Rodríguez Ballesteros
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * angel.rodriguez@esne.edu
 */

#include "Intro_Scene.hpp"
#include "Menu_Scene.hpp"
#include <basics/Canvas>
#include <basics/Director>

using namespace basics;
using namespace std;

namespace example
{

    // ---------------------------------------------------------------------------------------------

    bool Intro_Scene::initialize ()
    {
        if (state == UNINITIALIZED)
        {
            state = LOADING;
        }
        else
        {
            timer.reset ();

            logoNum = 0;
            opacity = 0.f;
            state   = FADING_IN;
        }
        return true;
    }

    // ---------------------------------------------------------------------------------------------

    void Intro_Scene::update (float time)
    {
        if (!suspended) switch (state)
            {
                case LOADING:    update_loading    (); break;
                case FADING_IN:  update_fading_in  (); break;
                case WAITING:    update_waiting    (); break;
                case FADING_OUT: update_fading_out (); break;
                default: break;
            }
    }

    // ---------------------------------------------------------------------------------------------

    void Intro_Scene::render (Graphics_Context::Accessor & context)
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

                if (EsneLogo_texture && CopterLogo_texture)
                {
                    canvas->set_opacity (opacity);

                    if(logoNum == 0){
                        canvas->fill_rectangle
                                (
                                        { canvas_width * .5f, canvas_height * .5f },
                                        { EsneLogo_texture->get_width (), EsneLogo_texture->get_height () },
                                        EsneLogo_texture. get ()
                                );
                    }
                    else{
                        canvas->fill_rectangle
                                (
                                        { canvas_width * .5f, canvas_height * .5f },
                                        { CopterLogo_texture->get_width (), CopterLogo_texture->get_height () },
                                        CopterLogo_texture. get ()
                                );
                    }
                }
            }
        }
    }

    void Intro_Scene::update_loading ()
    {
        Graphics_Context::Accessor context = director.lock_graphics_context ();

        if (context)
        {
            // Se carga la textura del logo de Esne:

            EsneLogo_texture = Texture_2D::create (0, context, "EsneLogo.png");

            // Se carga la textura del logo del Juego:

            CopterLogo_texture = Texture_2D::create (0, context, "CopterLogo.png");

            // Se comprueba si las texturas se han podido cargar correctamente:

            if (EsneLogo_texture && CopterLogo_texture)
            {
                context->add (EsneLogo_texture);
                context->add (CopterLogo_texture);

                timer.reset ();

                logoNum = 0;
                opacity = 0.f;
                state   = FADING_IN;
            }
            else
                state   = ERROR;
        }
    }

    void Intro_Scene::update_fading_in ()
    {
        float elapsed_seconds = timer.get_elapsed_seconds ();

        if (elapsed_seconds < 1.f)
        {
            opacity = elapsed_seconds;      // Se aumenta la opacidad del logo a medida que pasa el tiempo
        }
        else
        {
            timer.reset ();

            opacity = 1.f;
            state   = WAITING;
        }
    }

    void Intro_Scene::update_waiting ()
    {
        // Se esperan dos segundos sin hacer nada:

        if (timer.get_elapsed_seconds () > 2.f)
        {
            timer.reset ();

            state = FADING_OUT;
        }
    }

    void Intro_Scene::update_fading_out ()
    {
        float elapsed_seconds = timer.get_elapsed_seconds ();

        if (elapsed_seconds < .5f)
        {
            opacity = 1.f - elapsed_seconds * 2.f;      // Se reduce la opacidad de 1 a 0 en medio segundo
        }
        else
        {
            if(logoNum == 1){
                // Cuando el faceout se ha completado, se lanza la siguiente escena:
                logoNum = 0;

                state = FINISHED;

                director.run_scene (shared_ptr< Scene >(new Menu_Scene));
            }
            else{
                logoNum = 1;
                opacity = 0.f;
                timer.reset ();
                state = FADING_IN;
            }
        }
    }

}