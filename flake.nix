{
  description = "Blazingly fast, ultra-minimal system information tool for Linux";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  inputs.flake-utils.url = "github:numtide/flake-utils";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        packages.default = pkgs.stdenv.mkDerivation {
          pname = "zeptofetch";
          version = "2.1";
          src = self;

          # -march=native isn't reproducible in a Nix build; drop NATIVE.
          buildPhase = "make";
          installPhase = "make install PREFIX=$out";
        };

        devShells.default = pkgs.mkShell {
          packages = [ pkgs.gcc pkgs.gnumake ];
        };
      });
}
