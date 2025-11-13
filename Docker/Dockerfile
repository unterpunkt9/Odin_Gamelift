FROM ghcr.io/epicgames/unreal-engine:runtime

USER root
RUN apt-get update && apt-get install -y --no-install-recommends openjdk-17-jre-headless \
 && rm -rf /var/lib/apt/lists/*
USER ue4
# Put game under /local/game and make ue4 the owner
WORKDIR /local/game
COPY --chown=ue4:ue4 LinuxServer/ /local/game/

# Agent and entrypoint; also owned by ue4
COPY --chown=ue4:ue4 GameLiftAgent-1.0.jar /gamelift/agent.jar
COPY --chown=ue4:ue4 entrypoint.sh /entrypoint.sh
RUN chmod 0755 /entrypoint.sh \
 && mkdir -p /local/game/Saved/Logs /local/game/logs /local/gameliftagent \
 && chown -R ue4:ue4 /local /gamelift

# drop privileges – UE5 refuses to run as root


EXPOSE 7777/udp
ENTRYPOINT ["/bin/bash", "/entrypoint.sh"]