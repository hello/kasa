-- This is a test config file

inertia = {
  {1.1, 0.1, 0.2},
  {0.3, 1.2, 0.4},
  {0.5, 0.6, 1.3}
}

pelvis = {
  name = "pelvis",
  mass = 912393.,
  com = { 1.1, 1.2, 1.3},
  inertia = inertia
}
thigh_right = {
  name = "thigh_right",
  mass = 912393.,
  com = { 1.1, 1.2, 1.3},
  inertia = inertia
}

shank_right = {
  name = "shank_right",
  mass = 912393.,
  com = { 1.1, 1.2, 1.3},
  inertia = inertia
}

thigh_left = {
  name = "thigh_left",
  mass = 912393.,
  com = { 1.1, 1.2, 1.3},
  inertia = inertia
}

shank_left = {
  name = "shank_left",
  mass = 912393.,
  com = { 1.1, 1.2, 1.3},
  inertia = inertia
}

bodies = {
  pelvis = pelvis,
  thigh_right = thigh_right,
  shank_right = shank_right,
  thigh_left = thigh_left,
  shank_left = shank_left
}

joints = {
  freeflyer = {
    { 0., 0., 0., 1., 0., 0.},
    { 0., 0., 0., 0., 1., 0.},
    { 0., 0., 0., 0., 0., 1.},
    { 0., 0., 1., 0., 0., 0.},
    { 0., 1., 0., 0., 0., 0.},
    { 1., 0., 0., 0., 0., 0.}
  },
  spherical_zyx = {
    { 0., 0., 1., 0., 0., 0.},
    { 0., 1., 0., 0., 0., 0.},
    { 1., 0., 0., 0., 0., 0.}
  }
}

model = {
  name = "MyName",
  nested = {
    boolean = true
  },
  some = {
    nested = {
      boolean = true
    }
  },
  array = {
    true,
    false,
    true,
    false
  },
  group = {
    "Member.1",
    "Member_2",
    "Member-3",
    "Member+4",
  },
  sometext = "bla",
  somenumber = 213.,
  somearray = { 1.1, 2.2, 3.3, 4.4},
  somehashmap = {
    peter = 1.,
    john = 92.,
    bob = 923,
  },
  frames = {
    pelvis = {
      parent_body = nil,
      child_body = bodies.pelvis,
      joint = joints.freeflyer
    },
    thigh_right = {
      parent_body = bodies.pelvis,
      child_body = bodies.thigh_right,
      joint = joints.spherical_zyx
    },
  }
}

return model

