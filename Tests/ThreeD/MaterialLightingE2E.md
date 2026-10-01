# 3D Material / Lighting E2E

Editor 構成で実行し、3D Level を新規作成する。

1. GLB を Import して StaticMeshActor として配置する。Viewport で Picking し、Gizmo で位置・回転・XYZ が異なる Scale に変更する。
2. Inspector の Properties で ShadingModel を Lit にし、BaseColor と BaseColorTexturePath を設定する。DirectionalLightActor を配置し、Rotation、Color、Intensity、Enabled を変更する。
3. Light の Rotation と Intensity を変えると、Lit Mesh の明暗が変化することを確認する。Light を無効にした場合は Ambient のみで描画されることを確認する。
4. ShadingModel を Unlit に切り替え、Light の Rotation、Color、Intensity を変えても Mesh の見た目が変化しないことを確認する。
5. Level を保存して再読込する。Material と Light の値、および Actor の Transform が一致することを確認する。
6. Play を開始し、再読込した Scene の状態が保たれることを確認する。
7. 既存の 2D Level を開き、描画順と Texture 表示が従来どおりであることを確認する。

non-uniform Scale の判定では、同じ GLB を均一 Scale と非均一 Scale で並べ、対応する面の明暗が Scale によって不自然に変わらないことを確認する。
