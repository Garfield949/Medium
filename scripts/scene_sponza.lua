local camera = {
    fov = 75,
    near = 0.01,
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 0.0, 0.0},
}

local light = {
    near = 0.1,
    far  = 30,
    width = 20,
    height = 20,
    position = {1.0, 10.0, 0},
    lookat   = {-1.0, -1.0, 2.0}
}

local model = {
    name = "Crytek Sponza",
    file = "assets/models/sponza/sponza.obj",
    position = {0.0, 0.0, 0.0},
    rotation = {0.0, 90.0, 0.0},
    scale = 0.006
}

return { Camera = camera, Light = light, Model = model }