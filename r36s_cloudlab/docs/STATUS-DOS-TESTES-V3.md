# RE4NX dArkOSRE CloudLab v3: Fase 4 HIPC/CMIF (experimental)

Esta fase passa do IPC inventado da Fase 3 para a leitura de bytes com
campos do protocolo HIPC/CMIF. Referencia publica:
https://switchbrew.org/wiki/HIPC .

## Componentes implementados

- Parser limitado do cabecalho HIPC (8 bytes) e dos cabecalhos CMIF
  `SFCI` e `SFCO`;
- Resposta serializada com handle movido e padding, seguindo o formato
  basico documentado; service manager `sm:` somente **sintetico**;
- Rejeicao segura de descritores/buffers, dominios, specials, receive-lists
  e versoes ainda nao suportadas;
- Handles versionados, 10.000 ciclos de uso, 10 testes negativos,
  50.000 mutacoes deterministicas do parser;
- Compilacao de ELF Linux AArch64 estatico para Cortex-A35.

## Testes locais realizados (host x86_64)

- CloudLab CTest: 3/3;
- RE4NX CTest: 8/8;
- Phase4 ASAN e UBSAN: PASS com --selftest e --fuzz;
- Validador ABI: 3/3 ELF ARM64 estaticos (Fases 2, 3 e 4).

## Falta validar

- Execucao do ELF AArch64 no runner GitHub Actions e dArkOSRE;
- Mesa EGL/GLES3 no CI, kernel 4.4.189, driver Mali-G31 e FPS;
- SVC Switch reais, services HID/VI/NVDRV, loader de RE4 e NVN -> GLES3.

O codigo de Switch comercial **nao** e executado aqui. O parser nao e
um emulador funcional nem uma prova de que o RE4 cabera em 1 GB RAM.

## Status da nuvem

O workflow foi atualizado nesta branch de PR em rascunho, sem alterar
`master`. O GitHub exige workflow na branch default para disparo manual;
`push`/PR ainda nao geraram execucoes remotas verificaveis.

Referencias:
- https://switchbrew.org/wiki/HIPC
- https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow
