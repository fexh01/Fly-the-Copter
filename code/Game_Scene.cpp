/*
 * GAME SCENE
 * Copyright © 2022+ Félix Hernández Muñoz-Yusta
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * felixhernandezmy@gmail.com
 */

#include "Game_Scene.hpp"
#include "Menu_Scene.hpp"

#include <cstdlib>
#include <basics/Canvas>
#include <basics/Director>

using namespace basics;
using namespace std;

namespace flythecopter
{
    // ID y ruta de las texturas que se deben cargar para esta escena. La textura con el mensaje de
    // carga está la primera para poder dibujarla cuanto antes:

    Game_Scene::Texture_Data Game_Scene::textures_data[] =
            {
                { ID(loading),"game-scene/loading.png"},
                { ID(copter),"game-scene/helicoptero.png"},
                { ID(wall),"game-scene/wall.png"},
            };

    // Para determinar el número de items en el array textures_data, se divide el tamaño en bytes
    // del array completo entre el tamaño en bytes de un item:
    unsigned Game_Scene::textures_count = sizeof(textures_data) / sizeof(Texture_Data);



    Game_Scene::Game_Scene()
    {
        // Se establece la resolución virtual (independiente de la resolución virtual del dispositivo).
        // En este caso no se hace ajuste de aspect ratio, por lo que puede haber distorsión cuando
        // el aspect ratio real de la pantalla del dispositivo es distinto.
        canvas_width  = 1280;
        canvas_height =  720;

        // Se inicia la semilla del generador de números aleatorios:
        srand (unsigned(time(nullptr)));

        // Se inicializan otros atributos:
        initialize ();
    }


    // Algunos atributos se inicializan en este método en lugar de hacerlo en el constructor porque
    // este método puede ser llamado más veces para restablecer el estado de la escena y el constructor
    // solo se invoca una vez.
    bool Game_Scene::initialize ()
    {
        state     = LOADING;
        suspended = true;
        gameplay  = UNINITIALIZED;

        return true;
    }



    void Game_Scene::suspend ()
    {
        suspended = true;               // Se marca que la escena ha pasado a segundo plano
    }



    void Game_Scene::resume ()
    {
        suspended = false;              // Se marca que la escena ha pasado a primer plano
    }



    void Game_Scene::handle (Event & event)
    {
        if (state == RUNNING)               // Se descartan los eventos cuando la escena está LOADING
        {
            if (gameplay == GAME_OVER){     // En caso de tocar la pantalla una vez hayas perdido, vuelves al menu inicial
                switch (event.id) {
                    case ID(touch-ended):
                    {
                        director.run_scene(shared_ptr<Scene>(new Menu_Scene));
                        break;
                    }
                }
            }
            else if (gameplay == WAITING_TO_START)
            {
                start_playing ();           // Se empieza a jugar cuando el usuario toca la pantalla por primera vez
            }
            else switch (event.id)
                {
                    case ID(touch-started):         // El usuario toca la pantalla
                    {
                        flying = true;
                        break;
                    }
                    case ID(touch-moved):
                    {
                        flying = true;
                        break;
                    }

                    case ID(touch-ended):       // El usuario deja de tocar la pantalla
                    {
                        // En caso de tocar la esquina superior derecha, pausa el juego
                        if((*event[ID(x)].as< var::Float > ()) > canvas_width - 200 && (*event[ID(y)].as< var::Float > ()) > canvas_height - 150){
                            state = PAUSED;
                        }else{
                            flying = false;
                        }

                        break;
                    }
                }
        } else if(state == PAUSED){   // En caso de que el juego este pausado, si tocas la pantalla vuelve al juego
            state = RUNNING;
        }
    }


    //Este método se invoca automáticamente una vez por fotograma para que la escena actualize su estado.
    void Game_Scene::update (float time)
    {
        if (!suspended) switch (state)
            {
                case LOADING: load_textures  ();     break;
                case PAUSED: break;
                case RUNNING: run_simulation (time); break;
                case ERROR:   break;
            }
    }

    // Este método se invoca automáticamente una vez por fotograma para que la escena dibuje su contenido.
    void Game_Scene::render (Context & context)
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

                switch (state)
                {
                    case LOADING: render_loading   (*canvas); break;
                    case RUNNING: render_playfield (*canvas); break;
                    case PAUSED: render_pause (*canvas); break;
                    case ERROR:   break;
                }
            }
        }
    }


    // En este método solo se carga una textura por fotograma para poder pausar la carga si el
    // juego pasa a segundo plano inesperadamente. Otro aspecto interesante es que la carga no
    // comienza hasta que la escena se inicia para así tener la posibilidad de mostrar al usuario
    // que la carga está en curso en lugar de tener una pantalla en negro que no responde durante
    // un tiempo.
    void Game_Scene::load_textures ()
    {
        if (textures.size () < textures_count)          // Si quedan texturas por cargar...
        {
            // Las texturas se cargan y se suben al contexto gráfico, por lo que es necesario disponer
            // de uno:

            Graphics_Context::Accessor context = director.lock_graphics_context ();

            if (context)
            {
                // Se carga la siguiente textura (textures.size() indica cuántas llevamos cargadas):
                Texture_Data   & texture_data = textures_data[textures.size ()];
                Texture_Handle & texture      = textures[texture_data.id] = Texture_2D::create (texture_data.id, context, texture_data.path);

                // Se comprueba si la textura se ha podido cargar correctamente:
                if (texture) context->add (texture); else state = ERROR;

                BackButton_texture = Texture_2D::create (0, context, "volverMenu.png");
                CopterLogo_texture = Texture_2D::create (0, context, "CopterLogo.png");
                StopButton_texture = Texture_2D::create (0, context, "pause.png");
                Continue_texture   = Texture_2D::create (0, context, "continuar.png");

                if (BackButton_texture && CopterLogo_texture && StopButton_texture && Continue_texture)
                {
                    context->add (BackButton_texture);
                    context->add (CopterLogo_texture);
                    context->add (StopButton_texture);
                    context->add (Continue_texture);
                }else{
                    state = ERROR;
                }
            }
        }else if (timer.get_elapsed_seconds () > 1.f)   // Si las texturas se han cargado muy rápido
        {                                               // se espera un segundo desde el inicio de
            create_sprites ();                          // la carga antes de pasar al juego para que
            restart_game   ();                          // el mensaje de carga no aparezca y desaparezca demasiado rápido.
            state = RUNNING;
        }
    }

    // Creacion de los sprites del techo, el suelo y el jugador

    void Game_Scene::create_sprites ()
    {
        Sprite_Handle    top_bar(new Sprite( textures[ID(wall)].get () ));
        Sprite_Handle bottom_bar(new Sprite( textures[ID(wall)].get () ));

        top_bar->set_anchor   (TOP | LEFT);
        top_bar->set_position ({ 0, canvas_height });
        top_bar->set_size({ .0f + canvas_width, .0f + canvas_height / 15});

        bottom_bar->set_anchor   (BOTTOM | LEFT);
        bottom_bar->set_position ({ 0, 0 });
        bottom_bar->set_size({ .0f + canvas_width, .0f + canvas_height / 15});

        sprites.push_back (   top_bar);
        sprites.push_back (bottom_bar);


        Sprite_Handle player_handle(new Sprite( textures[ID(copter)].get () ));
        sprites.push_back ( player_handle);


        // Se guardan punteros a los sprites que se van a usar frecuentemente:
        top_border    =             top_bar.get ();
        bottom_border =          bottom_bar.get ();
        player   =  player_handle.get ();
    }


    // Cuando el juego se inicia por primera vez se llama a este método para restablecer la posición y velocidad de los sprites:
    void Game_Scene::restart_game()
    {
        player->set_position ({ canvas_width / 5.f, canvas_height / 2.f });
        player->set_speed_y  (0.f);

        gameplay = WAITING_TO_START;
    }



    void Game_Scene::start_playing ()
    {
        player->set_speed_y (-300.f); // Al jugador le afecta la gravedad

        gameplay = PLAYING;
    }



    void Game_Scene::run_simulation (float time)
    {
        // Se actualiza el estado de los sprites
        for (auto & sprite : sprites)
        {
            sprite->update (time);
        }
        if(gameplay == PLAYING){ // Mientras el juego esta en PLAYING, se crean obstaculos aleatorios

            if(rand() % 51 == 0 && timer.get_elapsed_seconds() > .75f){ //Probabilidad random de que aparezca obstaculo y tiempo que tiene que esperar hasta que salga el siguiente

                Sprite_Handle newObstacle(new Sprite( textures[ID(wall)].get () )); //Se crea el nuevo obstaculo

                //Se configuran sus propiedades (Posicion, velocidad,...)
                newObstacle->set_anchor(CENTER | RIGHT);
                newObstacle->set_position ({ canvas_width + 75, rand() % (canvas_height - 50) + (50)});
                newObstacle->set_size({ 75.f, 0.f + rand() % 200 + 100});
                newObstacle->set_speed_x(-400.f);

                //Se añade el nuevo obstáculo al array
                obstacles.push_back(newObstacle);

                //Se resetea el timer
                timer.reset();
            }

            // Se actualizan los obstáculos y si alguno sale de la pantalla se elimina del array
            for (auto & sprite : obstacles)
            {
                sprite->update (time);

                if(sprite->get_position_x() <= 0){
                    obstacles.pop_front();
                }
            }
        }
        //Se actualiza al jugador
        update_user ();

        //Se comprueban las colisiones con los obstáculos
        check_collisions ();
    }



    // Hace que el player vuele o no dependiendo de si el usuario está tocando.
    // Comprueba si colisiona con el techo y el suelo
    void Game_Scene::update_user ()
    {
        if(gameplay == GAME_OVER){
            player->set_speed_y(0.0f);
        } else if(gameplay == PLAYING){
            if (player->intersects (*top_border))
            {
                gameplay = GAME_OVER;
            }
            else if (player->intersects (*bottom_border))
            {
                gameplay = GAME_OVER;
            }
            else if (flying)
            {
                player->set_speed_y(350.f);
            }
            else
                player->set_speed_y (-300.f);
        }
    }

    // Se detectan las colisiones del jugador con los obstáculos
    void Game_Scene::check_collisions ()
    {
        if(gameplay == PLAYING){

            for (auto & sprite : obstacles)
            {
                if(sprite->intersects(*player)){
                    gameplay = GAME_OVER;
                }
            }
        }
    }


    //Muestra la pantalla de loading
    void Game_Scene::render_loading (Canvas & canvas)
    {
        Texture_2D * loading_texture = textures[ID(loading)].get ();

        if (loading_texture)
        {
            canvas.fill_rectangle
                    (
                            { canvas_width * .5f, canvas_height * .5f },
                            { loading_texture->get_width (), loading_texture->get_height () },
                            loading_texture
                    );
        }
    }


    // Se dibujan todos los sprites que conforman la escena.
    void Game_Scene::render_playfield (Canvas & canvas)
    {
        if(gameplay == PLAYING || gameplay == WAITING_TO_START){
            for (auto & sprite : sprites)
            {
                sprite->render (canvas);
            }
            for (auto & sprite : obstacles)
            {
                sprite->render (canvas);
            }
        }

        if(gameplay == PLAYING){
            canvas.fill_rectangle
                    (
                            { canvas_width * .9f, canvas_height * .85f },
                            { (StopButton_texture->get_width ())*.75f, (StopButton_texture->get_height ())*.75f },
                            StopButton_texture. get ()
                    );
        }
        //Muestra la pantalla de game over
        else if(gameplay == GAME_OVER){
            if (BackButton_texture && CopterLogo_texture){
                canvas.fill_rectangle
                        (
                                { canvas_width * .5f, canvas_height * .6f },
                                { (CopterLogo_texture->get_width ())*0.9f, (CopterLogo_texture->get_height ())*0.9f },
                                CopterLogo_texture. get ()
                        );
                canvas.fill_rectangle
                        (
                                { canvas_width * .5f, canvas_height * .25f },
                                { (BackButton_texture->get_width ()), (BackButton_texture->get_height ()) },
                                BackButton_texture. get ()
                        );
            }
        }

    }


    //Al pararse el juego se muestra un botón en grande para continuar
    void Game_Scene::render_pause (Canvas & canvas)
    {
        canvas.fill_rectangle
                (
                        { canvas_width * .5f, canvas_height * .5f },
                        { (Continue_texture->get_width ()), (Continue_texture->get_height ()) },
                        Continue_texture. get ()
                );
    }

}
