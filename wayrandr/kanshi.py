# import re
# from dataclasses import dataclass
# from typing import Optional
#
#
# @dataclass
# class OutputDirective:
#    criteria: str
#    directives: list[str]
#
#
# @dataclass
# class Profile:
#    outputs: list[OutputDirective]
#    exec: list[str] = []
#    name: Optional[str] = None
#
#
# @dataclass
# class KanshiConfig:
#    profiles: list[Profile]
#    includes: list[str]
#
#
# def parse_kanshi_config(file_path: str) -> KanshiConfig:
#    """
#    Parses an existing kanshi configuration file and returns a KanshiConfig object.
#    """
#    config = KanshiConfig()
#    current_profile = None
#
#    with open(file_path) as f:
#        lines = f.readlines()
#
#    profile_re = re.compile(r"profile\s*(\w+)?\s*\{")
#    output_re = re.compile(r"output\s+(.+?)(?:\s*\{|\s*(.+))?")
#    exec_re = re.compile(r"exec\s+(.+)")
#    include_re = re.compile(r"include\s+(.+)")
#
#    for line in lines:
#        line = line.strip()
#
#        # Handle includes
#        include_match = include_re.match(line)
#        if include_match:
#            config.includes.append(include_match.group(1))
#            continue
#
#        # Handle profile start
#        profile_match = profile_re.match(line)
#        if profile_match:
#            if current_profile:
#                config.profiles.append(current_profile)
#            current_profile = Profile(name=profile_match.group(1))
#            continue
#
#        # Handle output directives
#        output_match = output_re.match(line)
#        if output_match and current_profile:
#            criteria = output_match.group(1)
#            directives = output_match.group(2).split() if output_match.group(2) else []
#            current_profile.outputs.append(
#                OutputDirective(criteria=criteria, directives=directives)
#            )
#            continue
#
#        # Handle exec commands
#        exec_match = exec_re.match(line)
#        if exec_match and current_profile:
#            current_profile.exec_commands.append(exec_match.group(1))
#            continue
#
#        # Handle profile end
#        if line == "}":
#            if current_profile:
#                config.profiles.append(current_profile)
#                current_profile = None
#
#    return config
#
#
# def update_kanshi_config(config: KanshiConfig, profile_name: str, new_output: OutputDirective):
#    """
#    Updates the specified profile with a new output directive or adds
#     the profile if it doesn't exist.
#    """
#    for profile in config.profiles:
#        if profile.name == profile_name:
#            profile.outputs.append(new_output)
#            return
#
#    # If profile doesn't exist, add a new one
#    new_profile = Profile(name=profile_name, outputs=[new_output])
#    config.profiles.append(new_profile)
#
#
# def write_kanshi_config(file_path: str, config: KanshiConfig):
#    """
#    Writes the KanshiConfig object back to the config file without overwriting untouched lines.
#    """
#    lines_to_write = []
#
#    # Handle includes
#    for include in config.includes:
#        lines_to_write.append(f"include {include}")
#
#    # Handle profiles and their directives
#    for profile in config.profiles:
#        if profile.name:
#            lines_to_write.append(f"profile {profile.name} {{")
#        else:
#            lines_to_write.append("profile {")
#
#        for output in profile.outputs:
#            if output.directives:
#                lines_to_write.append(
#                  f'    output {output.criteria} {" ".join(output.directives)}')
#            else:
#                lines_to_write.append(f"    output {output.criteria} {{ }}")
#
#        for command in profile.exec_commands:
#            lines_to_write.append(f"    exec {command}")
#
#        lines_to_write.append("}")
#
#    # Write everything to file
#    with open(file_path, "w") as f:
#        f.write("\n".join(lines_to_write))
#
#
## Example Usage:
#
## Parse an existing config
## kanshi_config = "~/path/to/kanshi/config".replace("~", str(Path.home()))
## config = parse_kanshi_config(kanshi_config)
#
## Add a new output to a profile
## new_output = OutputDirective(criteria="HDMI-A-1", directives=["enable", "mode 1920x1080@60Hz"])
## update_kanshi_config(config, "nomad", new_output)
#
## Save the modified config back to the file
## write_kanshi_config("/path/to/kanshi/config", config)
