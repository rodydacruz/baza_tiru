extends Area2D


# Declare member variables here. Examples:
# var a = 2
# var b = "text"
var explosion_scene = preload("res://scenes/explosion.tscn")

var move_speed = 100.0
var score_emitted = false

signal score(points, asteroid_id)
var asteroid_id : int = randi()

# Called when the node enters the scene tree for the first time.
func _process(delta):
	position -= Vector2(move_speed * delta,0.0)
	if position.x <= - 100:
		queue_free()
	

func _on_asteroid_area_entered(area):
	if area.is_in_group("shot") or area.is_in_group("player") or area.is_in_group("low_nemy"):
		if not score_emitted:
			score_emitted = true
			emit_signal("score", 1, asteroid_id)
			queue_free()
			var stage_node = get_parent()
			var explosion_instance = explosion_scene.instantiate()
			explosion_instance.position = position
			stage_node.add_child(explosion_instance)
			
