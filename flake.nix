{
  description = "Blazingly fast, ultra-minimal system information tool for Linux";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "zeptofetch";
          version = "2.2";
          src = self;

          buildPhase = "make";
          installPhase = "make install PREFIX=$out";
        };

        devShells.default = pkgs.mkShell {
          packages = [ pkgs.gcc pkgs.gnumake ];
        };
      });
}
