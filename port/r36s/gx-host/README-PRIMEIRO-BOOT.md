# CubeEvil4 - primeiro boot experimental com dados originais no R36S

**Atenção: esta etapa não comprova jogabilidade ou desempenho.** O teste de
120 quadros anterior executou uma cena sintética sem ISO e passou. Este
executável separado `r36s_game_boot` tenta iniciar o *jogo verdadeiro* pela
interface existente `re4_coreproc`/IPC, passando os comandos ao renderizador
GX existente. Pode falhar, travar ou precisar de novos ajustes de memória.

## Pré-requisitos

O dispositivo precisa ter a instalação anterior completa
`/roms/ports/cubeevil4_live_gx`, com `re4_coreproc`,
`libre4_main.so`, `libre4_platform.so` e os 114 módulos.

É necessário extrair legalmente os **arquivos** de uma versão compatível do
Resident Evil 4 para GameCube (ID **G4BE08**, revisão exigida pelo projeto).
**Uma ISO colocada diretamente na pasta não funciona.** A extração pode ser
feita pelo menu *Extract Entire Disc* do Dolphin em um computador, por exemplo.
Não inclua ISO/arquivos do jogo nos issues, artifacts ou repositório público.

Copie seus arquivos extraídos para:

    /roms/ports/cubeevil4_live_gx/data/disc0/sys/boot.bin
    /roms/ports/cubeevil4_live_gx/data/disc0/sys/fst.bin
    /roms/ports/cubeevil4_live_gx/data/disc0/files/...

Se a extração nomeou a primeira mídia como `disc1`, ela também é aceita.
A segunda mídia deve usar numeração consistente (`disc1` se começou em
`disc0`, ou `disc2` se começou em `disc1`).

O programa verifica a existência de `boot.bin`, `fst.bin`, `files`
e a assinatura G4BE08 antes de iniciar o núcleo. **Isso não verifica
integridade nem revisão exata dos arquivos**: além desse check, a compatibilidade
precisa ser observada no boot real.

## Execução

1. Baixe o artifact `CubeEvil4-R36S-Primeiro-Boot-Experimental` da ação
   GitHub `R36S - first real-game boot frontend (experimental)`.
2. Extraia o ZIP em `/roms/ports` **sem apagar a pasta instalada anterior**.
3. Execute `RE4-Primeiro-Boot.sh` pelo menu Ports.

O teste tem limite de 45 segundos e usa OpenGL ES 3/KMSDRM.
Ele tenta ler controle via SDL_GameController e usa teclado como alternativa.
Áudio ainda é **descartado** (o IPC é drenado, sem reprodução no alto-falante).

O relatório será escrito em:

    /roms/ports/cubeevil4_live_gx/diagnostico_primeiro_boot_darkos.txt

Envie somente o relatório. Não é necessário enviar sua ISO nem arquivos do
jogo. Mesmo que o relatório mostre quadros, ainda precisaremos testar
interface, áudio, controles, save, RAM total e FPS.

O último teste comprovadamente funcional permanece disponível pelo launcher
`RE4-Core-GX-Ao-Vivo.sh`. Este pacote **não altera** o binário desse teste.
