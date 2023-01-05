SELECT DISTINCT trainer.name
FROM caughtpokemon, trainer
WHERE caughtpokemon.owner_id = trainer.id
AND caughtpokemon.pid = ANY (
        SELECT evolution.after_id AS final_id
        FROM evolution
        WHERE after_id <> ALL (
            SELECT evolution.before_id FROM evolution)
);