
#include "player.h"
#include "../controls/controller.h"
#include "../util/time.h"
#include "../defs.h"
#include "../physics/point_constraint.h"
#include "../math/mathf.h"

struct Vector3 gGrabDistance = {0.0f, 0.0f, -2.5f};
struct Vector3 gCameraOffset = {0.0f, 1.2f, 0.0f};

void playerInit(struct Player* player) {
    transformInitIdentity(&player->transform);
    player->grabbing = NULL;
    player->yaw = 0.0f;
    player->pitch = 0.0f;
}

#define PLAYER_SPEED    (5.0f)
#define PLAYER_ACCEL    (40.0f)
#define PLAYER_STOP_ACCEL    (80.0f)

#define ROTATE_RATE     (M_PI * 2.0f)
#define ROTATE_RATE_DELTA     (M_PI * 0.25f)
#define ROTATE_RATE_STOP_DELTA (M_PI * 0.25f)

void playerUpdate(struct Player* player, struct Transform* cameraTransform) {
    struct Vector3 forward;
    struct Vector3 right;

    quatMultVector(&player->transform.rotation, &gForward, &forward);
    quatMultVector(&player->transform.rotation, &gRight, &right);

    forward.y = 0.0f;
    right.y = 0.0f;

    vector3Normalize(&gForward, &gForward);
    vector3Normalize(&gRight, &gRight);

    OSContPad* controllerInput = controllersGetControllerData(0);

    struct Vector3 targetVelocity;

    vector3Scale(&forward, &targetVelocity, -controllerInput->stick_y * PLAYER_SPEED / 80.0f);
    vector3AddScaled(&targetVelocity, &right, controllerInput->stick_x * PLAYER_SPEED / 80.0f, &targetVelocity);

    vector3MoveTowards(
        &player->velocity, 
        &targetVelocity, 
        vector3Dot(&player->velocity, &targetVelocity) > 0.0f ? PLAYER_ACCEL * FIXED_DELTA_TIME : PLAYER_STOP_ACCEL * FIXED_DELTA_TIME, 
        &player->velocity
    );
    vector3AddScaled(&player->transform.position, &player->velocity, FIXED_DELTA_TIME, &player->transform.position);

    float targetYaw = 0.0f;
    float targetPitch = 0.0f;

    if (controllerGetButton(0, L_CBUTTONS)) {
        targetYaw += ROTATE_RATE;
    } else if (controllerGetButton(0, R_CBUTTONS)) {
        targetYaw -= ROTATE_RATE;
    }

    if (controllerGetButton(0, U_CBUTTONS)) {
        targetPitch += ROTATE_RATE;
    } else if (controllerGetButton(0, D_CBUTTONS)) {
        targetPitch -= ROTATE_RATE;
    }

    player->yawVelocity = mathfMoveTowards(
        player->yawVelocity, 
        targetYaw, 
        player->yawVelocity * targetYaw > 0.0f ? ROTATE_RATE_DELTA : ROTATE_RATE_STOP_DELTA
    );
    player->pitchVelocity = mathfMoveTowards(
        player->pitchVelocity, 
        targetPitch, 
        player->pitchVelocity * targetPitch > 0.0f ? ROTATE_RATE_DELTA : ROTATE_RATE_STOP_DELTA
    );

    player->yaw += player->yawVelocity * FIXED_DELTA_TIME;
    player->pitch = clampf(player->pitch + player->pitchVelocity * FIXED_DELTA_TIME, -M_PI * 0.5f, M_PI * 0.5f);

    quatAxisAngle(&gUp, player->yaw, &player->transform.rotation);

    struct Quaternion pitch;
    quatAxisAngle(&gRight, player->pitch, &pitch);
    quatMultiply(&player->transform.rotation, &pitch, &cameraTransform->rotation);

    transformPoint(&player->transform, &gCameraOffset, &cameraTransform->position);

    if (player->grabbing) {
        struct Vector3 grabPoint;
        transformPoint(cameraTransform, &gGrabDistance, &grabPoint);
        pointConstraintMoveToPoint(player->grabbing, &grabPoint, 20.0f);
        pointConstraintRotateTo(player->grabbing, &cameraTransform->rotation, 5.0f);
    }
}
