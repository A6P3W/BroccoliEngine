"""Generate temporary GLB/glTF API fixtures from the checked-in Fox model."""
import argparse
import copy
import json
from pathlib import Path
import struct


def WriteGLB(PathValue, Document, Binary):
    JsonChunk = json.dumps(Document, ensure_ascii=False).encode("utf-8")
    JsonChunk += b" " * (-len(JsonChunk) % 4)
    Binary += b"\0" * (-len(Binary) % 4)
    Data = struct.pack("<III", 0x46546C67, 2, 28 + len(JsonChunk) + len(Binary))
    Data += struct.pack("<II", len(JsonChunk), 0x4E4F534A) + JsonChunk
    Data += struct.pack("<II", len(Binary), 0x004E4942) + Binary
    PathValue.write_bytes(Data)


def Generate(Directory):
    Directory.mkdir(parents=True, exist_ok=True)
    Root = Path(__file__).resolve().parents[2]
    Data = (Root / "Engine/Resources/Model/Fox.glb").read_bytes()
    JsonSize = struct.unpack_from("<I", Data, 12)[0]
    Document = json.loads(Data[20:20 + JsonSize])
    BinaryOffset = 20 + JsonSize
    BinarySize = struct.unpack_from("<I", Data, BinaryOffset)[0]
    Binary = Data[BinaryOffset + 8:BinaryOffset + 8 + BinarySize]
    BindPose = copy.deepcopy(Document)
    BindPose.pop("animations")
    WriteGLB(Directory / "BindPose.glb", BindPose, Binary)
    Skinless = copy.deepcopy(BindPose)
    Skinless.pop("skins")
    for Node in Skinless["nodes"]:
        Node.pop("skin", None)
    for Mesh in Skinless["meshes"]:
        for Primitive in Mesh["primitives"]:
            Primitive["attributes"].pop("JOINTS_0", None)
            Primitive["attributes"].pop("WEIGHTS_0", None)
    WriteGLB(Directory / "Skinless.glb", Skinless, Binary)
    Names = copy.deepcopy(Document)
    Names["animations"][0].pop("name", None)
    Names["animations"][1]["name"] = "Walk"
    Names["animations"][2]["name"] = "Walk"
    WriteGLB(Directory / "Names.glb", Names, Binary)
    Single = copy.deepcopy(Document)
    # A legal single-key clip at time zero, retaining the source joint pose.
    Single["animations"] = Single["animations"][:1]
    Single["animations"][0]["name"] = "Single"
    SingleBinary = bytearray(Binary)
    for Sampler in Single["animations"][0]["samplers"]:
        for Key in ("input", "output"):
            Accessor = Single["accessors"][Sampler[Key]]
            Accessor["count"] = 1
        Input = Single["accessors"][Sampler["input"]]
        Input["min"] = [0.0]
        Input["max"] = [0.0]
        View = Single["bufferViews"][Input["bufferView"]]
        Offset = View.get("byteOffset", 0) + Input.get("byteOffset", 0)
        struct.pack_into("<f", SingleBinary, Offset, 0.0)
    WriteGLB(Directory / "Single.glb", Single, bytes(SingleBinary))
    (Directory / "キツネ.GLB").write_bytes(Data)
    Gltf = copy.deepcopy(Document)
    Gltf["buffers"][0]["uri"] = "Fox.bin"
    (Directory / "Fox.bin").write_bytes(Binary)
    (Directory / "Fox.gltf").write_text(
        json.dumps(Gltf, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8", newline="\n")
    print(Directory.resolve().as_posix())


if __name__ == "__main__":
    Parser = argparse.ArgumentParser(description=__doc__)
    Parser.add_argument("directory", type=Path)
    Options = Parser.parse_args()
    Generate(Options.directory)
