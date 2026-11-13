# Resident Evil 4 — R36S Demo 640×480 (unofficial, experimental)

> **Only a demonstration build, not a validated port or a commercial release.**
> Capcom owns Resident Evil 4. This fan project is not endorsed by Capcom.

## Native screen and performance policy

- **640×480 display output**, 4:3, directly to the R36S screen via SDL2 / GLES3.
- **No 50% / 75% EFB reduction**: upstream GX retains its original **1× GameCube EFB** dimensions (typically up to 640×528, distinct from 640×480 screen output).
- XFB presentation uses **GL_NEAREST** for a pixel-sharp result, rather than an extra sharpening/FXAA shader. This may make jagged edges more noticeable; original texture sampling remains unchanged.
- **No MSAA; anisotropic filtering 1×; FXAA, PSX and TV filter disabled** by the existing low-memory profile.
- The existing upstream potato mask **31** disables optional weather/particle/special effects and additional lighting/shadows during regular gameplay. This is visually aggressive, but it **does not remove collision, enemies, UI, logic or animations**. Use 0 to compare original effects.
- Cortex-A35 specific **-O3 + LTO** for the small SDL2/GX frontend; no fast-math. Preserve the tested Mali-G31 glFinish + VSync mode 5 to avoid graphics instability.
- Shader binaries cached on SD card when the driver supports GL_PROGRAM_BINARY; max active programs (256 regular + 64 merged) follows the existing low-memory policy. Shader cache can hitch or be unavailable with the firmware.
- Release frontend compiles **without crash tracing, debug profiler, FPS overlay or diagnostic symbols**; output is suppressed. The game's native PRINT_PAGE debug config is disabled in a temporary copied overlay; if this cannot be done the demo refuses to run (original files stay untouched).
- Start+Select hold ~2s or Start hold ~8s to leave; gamepad mapping is firmware dependent.

## Installation on dArkOSRE

The **GitHub Actions test ZIP** (if all build jobs pass) contains Linux ARM64 frontend and separately compiled matching core, but **does not contain any disc, art, sound, BIOS, saves or proprietary original game data**. For legitimate local tests, extract under `/roms/ports/`:

```
/roms/ports/Resident-Evil-4-R36S-DEMO-640x480.sh
/roms/ports/ResidentEvil4-R36S-Demo-640x480/
  r36s_re4_demo_640x480
  re4_coreproc
  libre4_main.so
  libre4_platform.so
  libre4_host.so
  modules/libre4_*.so
  data/disc0/                <-- add your own legitimate, compatible G4BE08 files
  launch-demo-640x480.sh
```

The **GitHub prerelease asset**, if publication succeeds, is **frontend/launcher only** and requires compatible user-built core modules from the same source revision. This conservative publication split avoids bundling reconstructed game binaries in the public release. For personal testing prefer the complete CI artifact instead.

No files are installed over existing `cubeevil4_live_gx` or `cubeevil4_0995_exp` ports. Saves and original disc remain yours.

## What still needs real hardware tests

CI can verify compilation, file structure, unit tests, ELF architecture and Mesa/EGL smoke checks. Only a physical R36S test confirms correct image, 640×480 video mode, game load, shader sharpness, audio, FPS/game speed, heat/RAM, menu controls and saving. The 30 FPS objective is not yet confirmed.

A "demo" here refers to an **unofficial technical demonstration**, not a shareable sample of copyrighted game content.

## Estado confirmado em 13/11/2026 e retomada

- Código e documentação salvos na branch `experiment/r36s-re4-demo-640x480-sharp` (PR #32 para a branch experimental 0.99.5).
- Compilação nativa ARM64 **ainda NÃO executada ou validada**. Verificações GitHub API mostraram zero runs novos; não há ZIP de demonstração nem Release publicados.
- Para disparar o CI caso commits do GitHub App não o iniciem: com a branch experimental selecionada, edite este documento na interface web do GitHub (adicione uma linha em branco ou comentário) e **Commit changes** diretamente nessa branch. O evento `push` gerado pela conta do proprietário deverá tentar iniciar `.github/workflows/r36s-demo-640x480.yml`. Alternativamente, dê push de commit vazio via git local (o workflow não usa filtro de paths).
- O pipeline inclui smoke-test EGL no host, teste ARM64 do frontend, compilação do core recompilado, ZIP para testes locais e GitHub prerelease **frontend-only** após sucesso integral. Erros exigem corrigir a etapa específica; não mesclar PR #29/#32 antes disso.
- Na próxima conversa: inspecionar os runs mais novos de `dehalbuq/CubeEvil4-Android`, a PR #32, o workflow acima e seu artifact `Resident-Evil-4-R36S-DEMO-640x480-ARM64-TEST`; se compilou, testar no R36S com dArkOSRE usando arquivos originais compatíveis G4BE08 e reportar FPS/áudio/saída.

## Início manual da compilação (GitHub Actions)

O arquivo `.github/workflows/build-re4-r36s-demo-manual.yml` foi criado em
`master` como **acionador de build, não como mesclagem do jogo**. Ele chama
o workflow reutilizável na branch
`experiment/r36s-re4-demo-640x480-sharp`, cujo checkout está fixado
na mesma branch. Dessa forma o código R36S permanece separado da master.

1. Abra `https://github.com/dehalbuq/CubeEvil4-Android/actions/workflows/build-re4-r36s-demo-manual.yml`.
2. Clique **Run workflow**, escolha `master` e confirme.
3. Acompanhe `host-smoke`, `frontend-arm64`, `matching-core-arm64`, `assemble` e `publish-prerelease`.
4. Se todos passarem, abra o artefato `Resident-Evil-4-R36S-DEMO-640x480-ARM64-TEST`.
5. A prévia pública, caso criada, contém só o **frontend** sem módulos de jogo.

Nenhuma execução manual foi disparada pelas ferramentas da sessão em
13/11/2026: elas não fornecem `workflow_dispatch`. A presença
do workflow no GitHub não significa que a compilação passou.

**Estado seguinte:** verificar run manual, diagnosticar job falhado com logs
e só então marcar o pacote como compilado; no R36S realizar teste físico.
