#include <xieps_demo.h>

XIStory*
demo_build_entire_story()
{
  g_debug(_("%s: Entered"), __FUNCTION__);

  /* Step #1 - Create a new story data structure  */
  XIStory *story = NULL;
  story = xi_story_new(.name          = "quinn_goes_hiking",
                       .revision      = "0.0.1",
                       .natural_w     = 800,
                       .natural_h     = 600,
                       .natural_bpp   = 32,
                       .scale_mode    = "stretch");
  xi_start_story_asap(story);

  g_message(_("%s: New story started with name: '%s'"),
            __FUNCTION__, story->name);

  XISequence *intro = xi_sequence_add_child(story->root_seq, "intro1",
                                            .start_asap=TRUE);
  intro->pos->x = 20;
  intro->pos->y = 30;
  xi_sequence_set_camera(intro, 0, 0, 0);

  xi_sequence_connect_input_to_camera_xy(intro);

  xi_drawable_add(story->root_seq,
                  .instance_name     = "root_bg",
                  .name              = "bluegreen_800x600.bmp",
                  .use_alpha         = FALSE,
                  .use_alpha_channel = FALSE);

  XISequence *hero = xi_sequence_add_child(intro, "hero",
                                           .start_at=2);
  hero->pos->x = 43;
  hero->pos->y = 34;

  XISequence *hero_walk =
    xi_sequence_add_child(hero, "hero_walk",
                          .duration=1.5,
                          .duration_type=XI_DURATION_TRUNCATE,
                          .start_asap=TRUE,
                          .start_on="hero_tip_hat:done", // TODO: This won't work because it hasn't been declared yet. Consider processing listeners when story starts starts.
                          .restartable=TRUE);

  XIDrawable *hero_d =
    xi_drawable_add(hero_walk,
                    .instance_name     = "mustaphacairo-img",
                    .name              = "mustaphacairo.png",
                    .use_alpha         = FALSE,
                    .use_alpha_channel = FALSE,
                    .use_colorkey      = TRUE,
                    .colorkey_red      = 0,
                    .colorkey_green    = 0,
                    .colorkey_blue     = 248);
  hero_d->pos->z = 5;

  XIDrawableFrames *walk
    = xi_drawable_add_drawable_frames(hero_d, "walk", 2);

  xi_drawable_frames_set(walk, 0, .x=365, .y=0, .h=95, .w=30, .duration=0.3);
  xi_drawable_frames_set(walk, 1, .x=400, .y=0, .h=95, .w=30, .duration=0.3);

  XISequence *hero_tip_hat = xi_sequence_add_child(hero, "hero_tip_hat",
                                                   .start_on="hero_walk:done");

  hero_d =
    xi_drawable_add(hero_tip_hat,
                    .instance_name     = "mustaphacairo-img",
                    .name              = "mustaphacairo.png",
                    .use_alpha         = FALSE,
                    .use_alpha_channel = FALSE,
                    .use_colorkey      = TRUE,
                    .colorkey_red      = 0,
                    .colorkey_green    = 0,
                    .colorkey_blue     = 248);
  hero_d->pos->z = 5;

  XIDrawableFrames *tip_hat
    = xi_drawable_add_drawable_frames(hero_d, "tip_hat", 6);

  xi_drawable_frames_set(tip_hat, 0, .x=000, .y=0, .h=95, .w=50, .duration=0.5);
  xi_drawable_frames_set(tip_hat, 1, .x=061, .y=0, .h=95, .w=60, .duration=0.05);
  xi_drawable_frames_set(tip_hat, 2, .x=107, .y=0, .h=95, .w=69, .duration=0.1);
  xi_drawable_frames_set(tip_hat, 3, .x=172, .y=0, .h=95, .w=67, .duration=0.75);
  xi_drawable_frames_set(tip_hat, 4, .x=236, .y=0, .h=95, .w=69, .duration=0.1);
  xi_drawable_frames_set(tip_hat, 5, .x=301, .y=0, .h=95, .w=60, .duration=0.05);

  gdouble tip_hat_duration = xi_drawable_frames_duration_calc(tip_hat);
  hero_tip_hat->duration = tip_hat_duration;
  hero_tip_hat->duration_type = XI_DURATION_TRUNCATE;
  g_debug(_("%s: tip_hat_duration=%g"), __FUNCTION__, tip_hat_duration);


  XIDrawable *mountain =
    xi_drawable_add(intro,
                    .instance_name     = "mountain-img",
                    .name              = "mountain.bmp",
                    .use_alpha         = TRUE,
                    .use_alpha_channel = FALSE,
                    .alpha             = 100,
                    .use_colorkey      = TRUE,
                    .colorkey_red      = 0,
                    .colorkey_green    = 0,
                    .colorkey_blue     = 255);
  mountain->pos->x = 300;
  mountain->pos->y = 150;
  mountain->pos->z = 5;

  XIDrawable *splash =
    xi_drawable_add(intro,
                    .instance_name     = "splash-img",
                    .name              = "tux-alpha.png",
                    .use_alpha_channel = TRUE);
  splash->pos->z = 10;

  // TODO: fade-to-black isn't working since 

  xi_fade_to_black(intro, "fade-out",
                   .duration=1,
                   .rate=10,
                   .start_at=1);

  xi_fade_from_black(intro, "fade-in",
                     .start_on="fade-out:done",
                     .rate=10,
                     .duration=1);

  xi_fade_to_black(intro, "fade-out-again",
                   .start_on="fade-in:done",
                   .rate=10,
                   .duration=1,
                   .start_at=1);
 
  g_debug(_("%s: ---------- END OF STORY BUILDING CODE ----------"),
          __FUNCTION__);

  /*
    xi_dump_sequence(story->root_seq);
    xi_dump_sequence(intro);
  */

  /* TODO: I Want to say:
     put drawable("tux.bmp") into intro
         at NOW, duration INFINITE
     put effect(fade_to_black) into intro
         at 3.0, duration 3.5

   */

  xi_dump_sequence(intro);

  g_debug(_("%s: Leaving"), __FUNCTION__);

  return story;


  /*
    xi_aef_crossfade(
    "scene_music_crossfade" // New instance name of crossfade effect
    "scene_music",  // Existing/new instance name of music that's playing
    "song2",        // New song to fade into
    3.5);           // Duration of crossfade
    xi_wait_for("fade_out");

    // New scene
    xi_new_scene(    // New scene. GC previous scene.
    "my_new_scene",  // New instance name for scene
    KEEP_PREV_SCENE
    | PRELOAD_NEXT_SCENE
    | DELETE_OTHER_SCENES);
    // Default behavior is to delete all previous scenes
    // and no preloading of next scene happens.

    xi_draw_composite(
    "backdrop",        // New instance name
    "scene1_backdrop", // Composite data name
    0.0, 0.0, 0.0,     // x, y, z
    "scenery"          // Interaction Layer group name
    );
    xi_draw_composite("quinn", "quinn_walking_lr",
    0.0, 0.0, 0.0, "activity"); // x, y, z, Interaction Layer group name
    xi_draw_movement(
    "quinn_entering",   // New instance name
    "quinn",            // Instance name of composite to move
    "enter_from_left + zigzag",  // Movement data to use
    5.0                 // Duration. Seconds to complete the movement.
    );
    xi_draw_composite("dog", "dog_walking_lr", 0.0, 0.0, 0.0, "activity");
    // The mimic feature allows you do define a movement on one character
    // and then have other characters copy their movement.
    xi_draw_movement_mimic("dog_following", "dog", "quinn");
    xi_draw_composite("stick", "stick_on_ground",
    40.0, 80.0, 0.0, "activity"); // x, y, z, Interaction Layer group name
    xi_vef_fade_from_black("fade_in");
    xi_wait_for("quinn_entering"); // Blocking? Non-blocking? Queued?
    xi_wait_for("fade_in");        // Wait for this too
    xi_stall(3.0); // Wait n seconds. Prev anim/movements continue.
    xi_draw_dialog(
    "text",            // New instance name for dialog
    xi_dialog_curr(),  // Text controlled by dialog tree system
    50.0, 70.0, 0.0, "dialog" // x, y, z, Interaction Layer group name
    );
    xi_wait_for("text");
    xi_draw_attach_two_composites_when(
    "stick@base[center]",   // Composite to be grabbed @ attachment point
    "quinn@hand[center]",   // Composite that grabs "stick"
    // Idea: quinn@hand[top-left|top-right|b-left|b-right|center|x,y]
    // Idea: "ATTACH stick (AT center OF base)
    //        TO quinn (AT center OF hand)
    //        immediately AFTER THESE EVENTS OCCUR:
    //          quinn's hand_ready,
    //          stick's base AND quinn's hand collide."
    // 'immediately' can be replaced by an interval: 3s 3.5s 50ms 10.5ms
    // 'base' and 'hand' are areas (rectangles) and center is a point
    // within those areas. 'center' could be an automatic point.
    // When 'hand_ready' and 'collide' events occur.
    "quinn[hand_ready] + (stick + quinn)[collide]"
    );
    // This time when "quinn" is drawn, because "quinn" was previous
    // placed in the scene, the existing instance values are inherited
    // for the new composite. That is x, y, z, interaction layer group name etc
    xi_draw_composite("quinn", "quinn_picking_up_item");
    xi_draw_user_interface(  // Draw next/back buttons etc.
    "ui",          // New instance name
    "page_turn_ui" // UI data
    );
    // This wait call may be implied, redundant and/or unnecessary
    xi_wait_for_user_interaction_from("ui"); // wait for UI by name

    // Next sequence depends on user's choice.

    */
}

int
main(int argc, char **argv)
{
  demo_build_entire_story();
}
