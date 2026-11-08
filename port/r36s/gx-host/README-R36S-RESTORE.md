# CubeEvil4 R36S — restaurar a instalação completa do port (binários ARM64)

Este pacote reúne **TUDO o que está disponível como compilação** do
port para DarkOSRE/R36S, sem baixar manualmente três ZIPs diferentes:

1. `CubeEvil4-R36S-Live-Core-GX` (core ARM64 e 114+ módulos)
   — compilação GitHub Actions 40114274602;
2. `CubeEvil4-R36S-GLES-Estavel-ARM64` (fallback conhecido)
   — compilação GitHub Actions 39994893655;
3. `CubeEvil4-R36S-Demo-Saida-Corrigida-ARM64`
   — compilação GitHub Actions 40114270888.

**O pacote não é uma ROM/ISO e não contém os arquivos originais
comerciais de Resident Evil 4.** O próprio usuário deverá restaurar
`cubeevil4_live_gx/data/disc0` da sua cópia legitimamente obtida
do GameCube **G4BE08**. Os módulos aqui são binários reconstruídos
pelo projeto e não substituem os dados extraídos do disco.

## Instalar sem apagar o que sobrou

1. **FAÇA BACKUP** da pasta `cubeevil4_live_gx/data/memcard/` se existir.
2. Abra a página da Action `R36S - restore compiled port binaries`.
3. Baixe o artifact `CubeEvil4-R36S-BINARIOS-RESTAURO-COMPLETO`.
4. Extraia o conteúdo do ZIP em `/roms/ports/`, **mesclando as
   pastas** `cubeevil4_live_gx/`. Não formate o cartão, não delete
   `data/` e não sobrescreva o memory card original.
5. No menu Ports, rode `RE4-R36S-Verificar-Arquivos.sh` para identificar
   exatamente quais arquivos ainda faltam.
6. Se o relatório apontar `data/disc0/sys/boot.bin`,
   `data/disc0/sys/fst.bin` ou `data/disc0/files/` como ausentes,
   restaure **essas pastas dos arquivos extraídos da SUA cópia** do
   Resident Evil 4 GameCube (ID G4BE08). Não substitua
   `data/memcard/` ao fazer essa restauração.
7. Abra `RE4-R36S-Demonstracao-Tela-Cheia.sh` para iniciar a demo
   fullscreen. A versão estável de diagnóstico poderá ser aberta pelo
   `RE4-R36S-Estavel.sh`.

## Árvore de arquivos do ZIP (somente compilação)

```text
/roms/ports/
  RE4-R36S-Demonstracao-Tela-Cheia.sh
  RE4-R36S-Estavel.sh
  RE4-Primeiro-Boot.sh
  RE4-R36S-Teste-10min.sh
  RE4-Core-GX-Ao-Vivo.sh
  RE4-R36S-Verificar-Arquivos.sh
  LEIA-ME-RESTAURO-PORT.md
  LEIA-ME-SAIDA-CORRIGIDA.md
  LEIA-ME-ESTAVEL.md
  cubeevil4_live_gx/
    r36s_game_showcase_exitfix      # demo mais recente
    r36s_game_boot                  # versao estavel de diagnostico
    r36s_live_ipc                   # teste de IPC, nao eh gameplay
    re4_coreproc
    libre4_main.so
    libre4_host.so
    libre4_platform.so
    modules/
      libre4_*.so                   # 114 ou mais
    README.md
    data/
      disc0/                        # NAO INCLUIDO: restaurar de sua copia
        sys/boot.bin
        sys/fst.bin
        files/
      memcard/                      # NAO INCLUIDO: restaurar backup do save
```

A pasta `data/` acima mostra os arquivos **esperados**, NÃO os
conteúdos do pacote. O ZIP não a cria nem a substitui.

## Qual demo executar

- **`RE4-R36S-Demonstracao-Tela-Cheia.sh`**:
  imagem preenchendo toda a tela 4:3, EFB 480×396, SDL áudio estéreo
  32kHz, debug do GameCube ocultado por config temporária,
  máximo duas horas por sessão, compilação Cortex-A35.
- Saída disponível no código: **segure START por 6 segundos**
  (sem precisar apertar A) ou **START+SELECT por 1,5 segundos**.
  Estes comandos foram implementados e compilados mas ainda precisam
  de confirmação de mapeamento físico no DarkOSRE.
- **`RE4-R36S-Estavel.sh`**:
  frontend de referência com registro de teste, duração limitada,
  usando `glFinish` e sincronização estável da Mali-G31.

Caso o START não funcione, ligue o diagnóstico de botão
`RE4_SHOWCASE_INPUT_DIAG=1 sh /roms/ports/RE4-R36S-Demonstracao-Tela-Cheia.sh`
e examine `cubeevil4_live_gx/logs/diagnostico_controles_demo_darkos.txt`.

## Dependências do sistema

Os binários ARM64 dependem das bibliotecas do **DarkOSRE**,
especialmente SDL2, EGL, GLESv2 e suporte de áudio; o pacote não
contém kernel, driver Mali, firmware, bibliotecas do sistema nem
PortMaster. Caso a instalação do sistema tenha sido danificada,
é necessário recuperar essas dependências da distribuição do
console; reinstalar os binários deste ZIP não conserta o sistema.

## Restrições e backup

- Restaurar apenas compilados do port é seguro para os arquivos
  `data/`: este ZIP foi criado com verificação explícita de que
  **nenhum arquivo `data/` ou `memcard/` é incluído**.
- Se um save `data/memcard/card-a.bin` foi excluído e não houver
  backup, ele não pode ser recuperado a partir do GitHub Actions.
- **O projeto/artefatos não conferem direitos de redistribuição
  do código reconstruído do jogo e arquivos protegidos.**
  O pacote destina-se à recuperação da instalação do próprio
  testador, não à distribuição pública de Resident Evil 4.
- Caso um dos artefatos de origem expire, o workflow de
  restauração precisará ser atualizado para usar novas builds
  ou recompilar do repositório.
