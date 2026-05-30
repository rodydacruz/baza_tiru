extends Control


var stage
var threat = null

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	stage = get_parent()
	
	
	
	
	pass # Replace with function body.

func getAngleBetweenTwoVector(vector1, vector2):
	return 0

func move_to_the_middle(delta,v1,v2):
	if (position.y - v1.y) < (v2.y - position.y):
		position.y += 1 * delta *100
		
	else:
		position.y -= 1 * delta *100
	pass
# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	
	if threat  != null && !stage.is_game_over:
		var player_possition = stage.getPlayerPosition()
		if player_possition.y > threat .position.y:
			move_to_the_middle(delta,player_possition,threat.position)
		else:
			move_to_the_middle(delta,threat .position,player_possition)
		rotation = getAngleBetweenTwoVector(threat , position)
	else:
		queue_free()
		
	pass
