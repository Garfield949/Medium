-- camear, light, model 为 table 类型，成员均为键值对。
local camera = {
    fov = 80,
    position = { 0.5, 2.0, 5.0 },
    rotation = { 0.0, -135.0, 0.0 },
}

local light = {
    direction = { -2.0, -15.0, 0.0 }
}

local shadow_config = {
    max_shadow_distance = 23.0,
    csm_cascade_count = 3
}

local model = {
    name = "Crytek Sponza",
    file = "assets/models/sponza/sponza.obj",
    position = { 0.0, 0.0, 0.0 },
    rotation = { 0.0, 90.0, 0.0 },
    scale = 0.006
}

-- 返回一个全局匿名 table. 以字符串 "Camera", "Light", "Model" 作为键名。
return {
    Camera = camera,
    Light = light,
    ShadowConfig = shadow_config,
    Model = model,
}
