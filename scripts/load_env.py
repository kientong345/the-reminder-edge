Import("env")
import os

env_path = os.path.join(env.subst("$PROJECT_DIR"), ".env")

if os.path.isfile(env_path):
    with open(env_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, val = line.split("=", 1)
            key = key.strip()
            val = val.strip().strip('"').strip("'")
            env.Append(CPPDEFINES=[(key, env.StringifyMacro(val))])
