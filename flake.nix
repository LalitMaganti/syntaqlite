{
  description = "Fast, accurate SQLite SQL formatter, validator, and language server";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      inherit (nixpkgs) lib;

      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];

      forAllSystems = lib.genAttrs systems;

      cliManifest = lib.importTOML ./syntaqlite-cli/Cargo.toml;
      workspaceManifest = lib.importTOML ./Cargo.toml;
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
        in
        {
          default = pkgs.rustPlatform.buildRustPackage {
            pname = "syntaqlite";
            version = cliManifest.package.version;

            src = self;

            cargoLock.lockFile = ./Cargo.lock;

            cargoBuildFlags = [
              "--package"
              cliManifest.package.name
            ];

            cargoTestFlags = [
              "--package"
              cliManifest.package.name
            ];

            meta = {
              description = cliManifest.package.description;
              homepage = workspaceManifest.workspace.package.homepage;
              license = lib.licenses.asl20;
              mainProgram = "syntaqlite";
              platforms = lib.platforms.unix;
            };
          };
        }
      );

      checks = forAllSystems (system: {
        default = self.packages.${system}.default;
      });

      overlays.default = final: _prev: {
        syntaqlite = self.packages.${final.stdenv.hostPlatform.system}.default;
      };
    };
}
