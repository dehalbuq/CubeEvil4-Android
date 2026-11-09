# R36S dArkOSRE CloudLab v3 — perfil real, testes automatizados e HIPC/CMIF

**Focado no RE4NX (Switch ARM64) e util para o CubeEvil4 (GameCube).**
Coleta dados tecnicos do dArkOSRE **sem root, sem instalar pacotes, sem
conexao a internet e sem capturar ROMs, chaves, jogos ou saves**. Reunimos
este coletor e um laboratorio reproduzivel para executar os testes sinteticos
das Fases 1–4 do RE4NX em ARM64 Linux, com OpenGL ES 3 por Mesa em modo software.

## Perfil real recebido e incorporado — nao precisa coletar novamente

O `darkosre-cloudprofile-20250904-075432.zip` foi validado: 68 de 68
checksums SHA-256 corretos; os scripts de coleta nao copiaram ROMs nem
bibliotecas proprietarias (`WITH_LIBS=0`). O arquivo bruto **nao e enviado
para o GitHub**: usamos somente o resumo tecnico em
`docs/R36S-DARKOSRE-REAL-PROFILE.json`.

* **CPU:** quatro ARM Cortex-A35 em RK3326, ate 1512 MHz, governor `performance`.
* **RAM:** 895 MiB informados pelo Linux, 695 MiB disponiveis no momento,
  sem swap. Teto recomendado para trabalhar inicialmente: **512 MiB RSS**;
  teste opcional ate 640 MiB. O orcamento virtual anterior de 768 MiB
  **nao e uma garantia de memoria fisica disponivel**.
* **GPU:** dispositivo `ff400000.gpu`, 520 MHz, governor `performance`;
  framebuffer 640x480p59. Mali-G31/ES3.2 confirmados apenas pelo log anterior
  do CubeEvil4, nao por uma consulta EGL nesta coleta.
* **SO:** kernel 4.4.189; userspace Debian 13 (trixie), SDL2 2.32.x,
  paginas de 4096 bytes; `GO-Super Gamepad` e `rk29-keypad` identificados.
* **Relogio:** o nome do ZIP tem data anterior a data de compilacao registrada
  no kernel, portanto nao confiamos no relogio do console para timestamps.

Os testes v2 incluem 6 regressoes de integridade da coleta, 6 testes
sinteticos RE4NX no PC, mais 9 casos negativos/positivos do NSO0 e
3 casos de IPC/handles; 3 niveis de memoria (64/256/512 MiB) com leitura
real de paginas, verificacao estatica do ELF AArch64, e compilacao de
GLSL ES3 via Mesa **se as dependencias estiverem instaladas**. No GitHub
Actions ARM64 o GLES3 e obrigatorio e a suite Phase2/3 executa nativamente.

## Como repetir a coleta no R36S (opcional)

1. Extraia o ZIP **mantendo a estrutura** para `/roms/ports/`. A raiz deve
   conter `R36S-Coletar-DarkOSRE.sh` e a pasta `r36s_cloudlab/`.
2. Pelo menu Ports, rode `R36S-Coletar-DarkOSRE.sh`. Nao pede root.
3. Em `/roms/ports/` surgira um arquivo `darkosre-cloudprofile-AAAAMMDD-HHMMSS.zip`
   (ou `.tar.gz` se seu firmware nao incluir o comando `zip`).
4. **Revise** o pacote e envie-o aqui; ele nao faz nenhum upload por conta propria.

Se nao aparecer no menu Ports, use o gerenciador SSH/terminal:

```sh
sh /roms/ports/R36S-Coletar-DarkOSRE.sh
```

Somente se desejar coletar **logs dos nossos ports**, rode:

```sh
sh /roms/ports/R36S-Coletar-DarkOSRE.sh --with-port-logs
```

Para guardar uma copia de bibliotecas de sistema (EGL/GLES/SDL2/Mali, quando
presentes), existe `--with-libs`, **desligado por padrao**. Aviso: bibliotecas
podem ser proprietarias. Nao distribua nem publique sem verificar sua licenca.
Nao e necessario usar essa opcao inicialmente.

## Informacoes coletadas

- arquitetura, versao do kernel e identificadores *genericos* do device tree
  (model e compatible, **sem serial**);
- CPU e hardware threads, frequencias e governors, temperaturas e devfreq;
- RAM, swap, pressao de memoria, pagina de memoria e ulimit;
- GPU/DRM, nos `/dev/dri`, Mali, EGL/GLES via eglinfo se instalado;
- dispositivos e mapeamentos Linux de input e bibliotecas SDL2;
- versoes de compiladores e dependencias, caminhos/versoes/hash das libs;
- perfil `.env` legivel por maquina com a meta de configuracao no CI;
- manifesto SHA-256 que permite verificar se o ZIP sofreu alteracoes.

Nao coletamos enderecos IP/MAC, SSID, serial, conteudo de jogos, credenciais,
chaves do Switch, NAND ou rootfs completa. Dados como versoes, dispositivos
e caminhos locais ainda podem revelar o sistema; revise antes de compartilhar.

## Rodar os testes no PC sem esperar hardware

A pasta `r36s_cloudlab/re4nx` contem o codigo-fonte experimental das Fases 1–4,
sem binarios do jogo. `cloud/run-cloud-tests.sh` compila o RE4NX e executa
sua bateria de testes sinteticos, alem do teste de W^X, threads e
memoria com leitura ate 512 MiB.

```sh
cd r36s_cloudlab
sh cloud/run-cloud-tests.sh
```

Se voce tiver um perfil gerado no R36S:

```sh
sh cloud/run-cloud-tests.sh /caminho/para/darkosre-cloudprofile-...zip
```

No Linux com `libegl-dev`, `libgles-dev` e Mesa instalados, o CI tambem
abre um contexto EGL/OpenGL ES 3 em modo software, valida shaders GLSL ES
3.00 e le um pixel. Isso confirma a API **naquele host**, nao o driver Mali.

**GitHub Actions:** o pacote inclui `.github/workflows/darkosre-cloudlab.yml`.
Ao adicionar `r36s_cloudlab/` e esse workflow na raiz de um repositorio
GitHub, voce pode disparar `Run workflow` no Actions. O job usa
`ubuntu-24.04-arm`: Linux ARM64 nativo, executa os binarios Phase2/3 e
verifica os resultados automaticamente, guardando logs como artifacts.
Ele nao precisa do ZIP pessoal `darkosre-cloudprofile` no repositorio publico:
usamos apenas JSON tecnico minimizado, que deve ser revisto antes de publicar.
O job tambem compila e executa novamente em um container **Debian trixie** com
limite estrito de 895 MiB de cgroup e sem swap; o kernel continua sendo o do host.

**IMPORTANTE**: o workflow esta apenas dentro do ZIP. Ele **nao foi instalado
nem executado no GitHub** por esta entrega. Para ativar CI, coloque a pasta
`r36s_cloudlab/` na raiz do seu repositorio e copie o workflow do pacote para
`.github/workflows/darkosre-cloudlab.yml` na raiz do repositorio. Entao faca
commit, e abra Actions -> R36S dArkOSRE CloudLab -> Run workflow. Se o GitHub
nao exibir o fluxo em branches, verifique se a instalacao de workflows na
branch padrao e obrigatoria nas configuracoes do repositorio.

**Docker local (se quiser):** execute na pasta `r36s_cloudlab`:

```sh
docker buildx build --platform linux/arm64 -t r36s-cloudlab -f cloud/Dockerfile --load .
docker run --rm --platform linux/arm64 --memory=895m --memory-swap=895m --cpus=4 r36s-cloudlab
```

Na maquina x86_64, a execucao ARM64 por QEMU pode ser mais lenta. A restricao
de memoria do Docker ajuda a detectar consumo excessivo, mas **nao simula**
a velocidade do RK3326, o driver Mali-G31 ou o kernel 4.4.189.

## Resultado ja validado nesta criacao

- Coletor shell executado em ambiente Linux de desenvolvimento, ZIP gerado.
- Leitor do ZIP verificou SHA-256 para todos os arquivos listados.
- Testes negativos: arquivo sensivel no ZIP detectado e recusado.
- Seis testes CTest do codigo RE4NX Phase1/2/3 passaram no PC x86_64.
- Testes CPU/mmap/W^X/64, 256 e 512 MiB passaram no PC x86_64.
- Codigo ARM64 Phase2/3 compilado como ELF AArch64; execucao nativa nos
  runners GitHub ARM64 ainda depende de um workflow registrado e executado.
- Teste Mesa GLES3 nao rodou neste PC porque nao ha headers EGL/GLES
  instalados; o GitHub Actions instala as dependencias antes de testar.

**O perfil real do R36S ja esta incorporado** e validado; nao ha necessidade
de repetir essa coleta para testar as fases sinteticas. Nao confundimos
resultado do PC/CI com desempenho real da Mali-G31.

## Limites e seguranca

Consulte `docs/KNOWN-LIMITATIONS.md`. Nunca coloque a pasta `sysroot/` gerada
com `--with-libs` em um repositorio publico. O CloudLab nao executa o jogo
comercial, nao recupera firmware ou chaves do Switch e nao roda emulacao NVN.


## Atualizacao v3: parser HIPC/CMIF da Fase 4

A Fase 4 agora inclui `re4nx/src/native_phase4.c`, um parser limitado
para a estrutura de cabecalho HIPC e CMIF do Switch, construido usando
informacoes publicas de https://switchbrew.org/wiki/HIPC .

O parser rejeita explicitamente descritores, regioes de buffer, dominio,
receive-list e mensagens especiais ainda nao implementadas. O unico
servico funcional e um **modelo sintetico** de `sm:`, usado para testar
abertura e encerramento seguro de handles e a serializacao SFCO.
Os demais nomes (`hid`, `vi:u`, `nvdrv`, `fsp-srv`, `appletOE`) possuem
handles de teste, mas **nao implementam** os seus comandos reais.

No host: CTest inclui dois testes novos, `phase4_hipc_cmif_wire` e
`phase4_hipc_fuzz_50k`. O sanitizador ASAN/UBSAN tambem foi executado
localmente no parser e passou. O nucleo Linux AArch64 estatico e gerado por
`sh re4nx/tools/build-native-phase4-aarch64.sh`.

A suite completa local passou em x86_64: 3 testes CloudLab e 8 testes
RE4NX CTest; o executavel ARM64 compilou e passou no check do ELF, mas **nao
foi executado em ARM64 no CI**. O teto de 895 MiB vem do perfil real do
console, nao de uma emulacao de tempo de CPU/GPU. O OpenGL ES3 por Mesa ainda
aguarda um runner que tenha EGL/GLES3 instalado.

Resumo tecnico em `docs/STATUS-DOS-TESTES-V3.md`.
