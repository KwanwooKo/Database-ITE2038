SELECT DISTINCT trainer.name, trainer.hometown
FROM trainer
JOIN caughtpokemon ON caughtpokemon.owner_id = trainer.id
JOIN pokemon ON pokemon.id = caughtpokemon.pid
JOIN gym ON gym.leader_id = trainer.id
WHERE pokemon.id IN (
        SELECT after_id AS final_id
        FROM evolution
        WHERE after_id NOT IN (
            SELECT before_id
            FROM evolution))
OR pokemon.id NOT IN (
        SELECT DISTINCT pokemon.id
        FROM pokemon, evolution
        WHERE pokemon.id = evolution.before_id
        OR pokemon.id = evolution.after_id
        ORDER BY pokemon.id)
ORDER BY trainer.name;