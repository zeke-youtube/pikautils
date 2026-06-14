# PikaUtils

PikaUtils is a Geometry Dash Geode mod with useful extras, gameplay polish, and a little personality.

## Features

- Dynamic Discord Rich Presence for menus, gameplay, practice mode, daily levels, weekly demons, and the level editor.
- Live Discord RPC toggle through Geode settings.
- Oiia Cat death sound effect.
- Click Between Frames setting for queued player inputs between physics steps.

## Settings

- `Enable Discord Rich Presence`: toggles Discord RPC without restarting Geometry Dash.
- `Click Between Frames`: queues player inputs through Geometry Dash's input command queue.

## Building

Requirements:

- Geometry Dash `2.2081`
- Geode `5.7.1`
- Geode CLI
- `GEODE_SDK` environment variable pointing to your Geode SDK checkout

Build with:

```sh
geode build
```

## Resources

- [Geode SDK Documentation](https://docs.geode-sdk.org/)
- [Geode SDK Source Code](https://github.com/geode-sdk/geode/)
- [Geode CLI](https://github.com/geode-sdk/cli)
- [Bindings](https://github.com/geode-sdk/bindings/)
