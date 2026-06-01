{
  description = "ESP32 3D Sensor Dashboard";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      pythonEnv = pkgs.python3.withPackages (
        ps: with ps; [
          flask
          requests
        ]
      );

    in
    {
      # ── `nix run` ────────────────────────────────────────────────────
      packages.${system}.default = pkgs.writeShellApplication {
        name = "esp32-dashboard";
        runtimeInputs = [ pythonEnv ];
        text = ''
          python ${./server.py}
        '';
      };

      # ── `nix develop` ────────────────────────────────────────────────
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pythonEnv ];
        shellHook = ''
          echo "ESP32 Dashboard dev shell ready."
          echo "Run:  python server.py"
        '';
      };
    };
}
