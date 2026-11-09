# R36S dArkOSRE CloudLab v2 — perfil real e testes automatizados

**Focado no RE4NX (Switch ARM64) e util para o CubeEvil4 (GameCube).**
Coleta dados tecnicos do dArkOSRE **sem root, sem instalar pacotes, sem
conexao a internet e sem capturar ROMs, chaves, jogos ou saves**. Reunimos
este coletor e um laboratorio reproduzivel para executar os testes sinteticos
das Fases 1–3 do RE4NX em ARM64 Linux, com OpenGL ES 3 por Mesa em modo software.

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

A pasta `r36s_cloudlab/re4nx` contem o codigo-fonte original das Fases 1–3,
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

**ESTADO ATUAL NO GITHUB**: o codigo-fonte, os testes e o workflow
`.github/workflows/darkosre-cloudlab.yml` foram publicados na branch
`experiment/re4nx-r36s-cloudlab-v2`, isolada da branch master, em
https://github.com/dehalbuq/CubeEvil4-Android/pull/30 .
Os testes locais passaram, mas o GitHub Actions **ainda nao executou**
nenhum job dessa branch. O workflow pode nao aparecer no menu Run workflow
enquanto ele nao existir na branch padrao. Nao confundir instalacao do
workflow com execucao ou aprovacao em ARM64 na nuvem.

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
